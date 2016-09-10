#include <fstream>
#include <time.h>  //clock

#include "compiler2.hpp"
#include "scriptengine.hpp"
#include "logger.hpp"
#include "mathlib.hpp"
#include "atomics.hpp"
#include "parser.hpp"
#include "generics.hpp"

using namespace std;

namespace NaReTi
{
	ScriptEngine::ScriptEngine(const std::string& _scriptLoc) :
		m_compiler(new codeGen::Compiler()),
		m_basicModule(new lang::BasicModule(m_compiler->getRuntime())),
		m_parser(new par::Parser()),
		m_moduleLoader(m_config)
	{
		m_compiler->compile(*m_basicModule);
		m_basicModule->initConstants();
		m_config.scriptLocation = _scriptLoc;
		//init globals
		//the initialization order follows the dependencies and should not be changed
	//	par::g_genericsParser = new par::GenericsParser();
		m_genericsParser = new par::GenericsParser(m_moduleLoader);
		par::g_genericsParser = m_genericsParser;
		//g_module is set in BasicModule::BasicModule

		//setup std math lib
		lang::MathModule* module = new lang::MathModule();
		loadModule("math", module);
		module->linkExternals();
		m_modules.emplace_back(module);
	}

	ScriptEngine::~ScriptEngine()
	{
		delete par::g_genericsParser;

		delete m_compiler;
		delete m_parser;
		delete m_basicModule;
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

	bool ScriptEngine::loadModule(const string& _fileName, NaReTi::Module* _dest)
	{
		string fileContent = m_moduleLoader.load(_fileName);
		if (fileContent == "#") return false;


		//use it without the file ending as name for the module
		string packageName = extractName(_fileName);
		if (!_dest)
			m_modules.emplace_back(new NaReTi::Module(packageName));
		NaReTi::Module& module = _dest ? *_dest : *m_modules.back();

		//look for dependencies
		m_parser->preParse(fileContent, module);
		auto dep = m_parser->getDependencies();
		for (auto& modName : dep)
		{
			Module* mod = getModule(modName);
			if (mod) module.m_dependencies.push_back(mod);
			else
			{
				LOG(Warning, "Could not load depended module: " << modName);
			}
		}
		bool ret = m_parser->parse(fileContent, module);
		if (ret)
		{
			//check dependencies again(generics can produce code)
		//	for (auto dep : module.m_dependencies)
		//		if (dep->hasChanged()) reloadModule(dep);
			if(m_config.optimizationLvl > None) m_optimizer.optimize(module);
			m_compiler->compile(module);
		}
		else
		{
			m_modules.pop_back();
			LOG(Error, "The module \"" << packageName << "\" could not be loaded due to a parsing error");
		}

		return ret;
	}

	bool ScriptEngine::loadModule(NaReTi::Module* _module)
	{
		return loadModule(_module->m_name, _module);
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

	bool ScriptEngine::reloadModule(const std::string& _moduleName)
	{

		return unloadModule(_moduleName, false) && loadModule(_moduleName);
	}

	bool ScriptEngine::reloadModule(NaReTi::Module* _module)
	{
		//dirty hack to keep pointers valid
		string name = _module->m_name;
		(*_module).~Module();
		new (_module)Module(name);
		return loadModule(_module);
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

	string ScriptEngine::extractName(const std::string& _fullName)
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
}