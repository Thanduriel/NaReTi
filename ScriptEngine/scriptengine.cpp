#include <fstream>
#include <time.h>  //clock

#include "compiler2.hpp"
#include "scriptengine.hpp"
#include "logger.hpp"
#include "mathlib.hpp"
#include "atomics.hpp"
#include "parser.hpp"
#include "generics.hpp"
#include "optimizer.hpp"
#include "stringlib.hpp"

using namespace std;

namespace NaReTi
{
	ScriptEngine::ScriptEngine(const Config& _config) :
		m_compiler(new codeGen::Compiler()),
		m_optimizer(new codeGen::Optimizer()),
		m_basicModule(new lang::BasicModule(m_compiler->getRuntime())),
		m_parser(new par::Parser()),
		m_config(_config),
		m_moduleLoader(m_config),
		m_threadPool(m_config.numThreads)
	{
		m_compiler->compile(*m_basicModule);
		m_basicModule->initConstants();
		//init globals
		//the initialization order follows the dependencies and should not be changed
	//	par::g_genericsParser = new par::GenericsParser();
		m_genericsParser = new par::GenericsParser(m_moduleLoader);
		par::g_genericsParser = m_genericsParser;
		//g_module is set in BasicModule::BasicModule

		//setup string
		lang::StringModule* strMod = new lang::StringModule();
		loadModule(*strMod);
		strMod->buildFunctions();
		m_modules.emplace_back(strMod);

		//setup std math lib
		lang::MathModule* module = new lang::MathModule();
		loadModule(*module);
		module->linkExternals();
		m_modules.emplace_back(module);
	}

	ScriptEngine::~ScriptEngine()
	{
		delete par::g_genericsParser;

		delete m_compiler;
		delete m_parser;
		delete m_basicModule;
		delete m_optimizer;
	}

	// ******************************************************* //

	Module* ScriptEngine::getModule(const string& _name)
	{
		for (auto& mod : m_modules)
		{
			if (mod->m_name == _name) return mod.get();
		}
		return loadModule(_name) ? getModule(_name) : nullptr;
	}

	// ******************************************************* //

	bool ScriptEngine::loadModule(const std::string& _name)
	{
		NaReTi::Module* mod = findModule(_name);
		if (!mod)
		{
			m_modules.emplace_back(new NaReTi::Module(_name));
			mod = m_modules.back().get();
		}
		return loadModule(*mod);
	}

	bool ScriptEngine::loadModule(NaReTi::Module& _module)
	{
		string fileContent = m_moduleLoader.load(_module.m_name);
		return loadModule(_module, fileContent);
	}

	bool ScriptEngine::loadModule(NaReTi::Module& _module, const std::string& _text)
	{
		rebuildModule(&_module); //clean up first

		if (_text == "#") return false;

		//look for dependencies
		m_parser->preParse(_text, _module);
		auto dep = m_parser->getDependencies();
		for (auto& modName : dep)
		{
			Module* mod = getModule(modName);
			if (mod) _module.m_dependencies.push_back(mod);
			else
			{
				LOG(Warning, "Could not load depended module: " << modName);
			}
		}
		bool ret = m_parser->parse(_text, _module);
		if (ret)
		{
			//check dependencies again(generics can produce code)
			//	for (auto dep : module.m_dependencies)
			//		if (dep->hasChanged()) reloadModule(dep);
			if (m_config.optimizationLvl > None) m_optimizer->optimize(_module);
			m_compiler->compile(_module);
		}
		else
		{
			unloadModule(_module.m_name);
			LOG(Error, "The module \"" << _module.m_name << "\" could not be loaded due to a parsing error");
		}

		return ret;
	}

	// ******************************************************* //

	bool ScriptEngine::unloadModule(const std::string& _moduleName, bool _keepBinary)
	{
		for (auto i = m_modules.begin(); i != m_modules.end(); ++i)
		{
			Module& module = *(*i);
			if (module.m_name == _moduleName)
			{
				if (!_keepBinary) m_compiler->release(module);
				m_modules.erase(i);
				return true;
			}
		}
		return false;
	}

	// ******************************************************* //

	FunctionHandle ScriptEngine::getFuncHndl(const std::string& _name) const
	{
		for (auto& module : m_modules)
			for (auto& func : module->m_functions)
				if (func->name == _name) return FunctionHandle(func->binary);

		return FunctionHandle(); // an null handle
	}

	// ******************************************************* //

	void ScriptEngine::call(FunctionHandle _hndl) const
	{
		assert((bool)_hndl); // not a valid handle

		((basicFunc*)_hndl.ptr)();
	}

	string ScriptEngine::extractName(const std::string& _fullName) const
	{
		//extract file name
		size_t endInd = _fullName.size();
		size_t beginInd = 0;
		size_t i = _fullName.size();
		while (i != 0)
		{
			i--;
			if (_fullName[i] == '.') endInd = i;
			else if (_fullName[i] == '\\' || _fullName[i] == '/')
			{
				beginInd = i + 1;
				break;
			}
		}
		//use it without the file ending as name for the module
		return _fullName.substr(beginInd, endInd);
	}

	void ScriptEngine::rebuildModule(NaReTi::Module* _module)
	{
		m_compiler->release(*_module);
		string name = _module->m_name;
		//dirty hack to keep pointers valid
		(*_module).~Module();
		new (_module)Module(name);
	}

	NaReTi::Module* ScriptEngine::findModule(const std::string& _name)
	{
		auto it = std::find_if(m_modules.begin(), m_modules.end(), [&](const std::unique_ptr<Module>& _ptr)
		{
			return _ptr->m_name == _name;
		});
		
		return it != m_modules.end() ? it->get() : nullptr;
	}
	// ******************************************************* //

	void ScriptEngine::checkTasks()
	{
		clock_t end = clock();
		// clean up finished tasks
		
		auto it = std::remove_if(m_tasks.begin(), m_tasks.end(), [](const TaskRestriction& _tr)
		{
			return (bool)_tr.task->isDone;
		});
		m_tasks.erase(it, m_tasks.end());

		for (auto& tr : m_tasks)
		{
			if (double(end - tr.begin) / CLOCKS_PER_SEC > tr.maxTime)
			{
				cout << "Task takes longer then allowed." << endl;
				m_threadPool.resetThread(0);
			}
		}
	}
}