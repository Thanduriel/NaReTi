#include <fstream>
#include <time.h>  //clock

#include "compiler2.hpp"
#include "scriptengine.h"
#include "logger.hpp"
#include "mathlib.hpp"
#include "atomics.hpp"
#include "parser.hpp"

using namespace std;

namespace NaReTi
{
	ScriptEngine::ScriptEngine():
		m_compiler(new codeGen::Compiler()),
		m_basicModule(new lang::BasicModule(m_compiler->getRuntime())),
		m_parser(new par::Parser())
	{
		m_config.scriptLocation = "../scripts/";

		//setup std math lib
		lang::MathModule* module = new lang::MathModule();
		loadModule("math.nrt", module);
		module->linkExternals();
		m_modules.emplace_back(module);
	}

	ScriptEngine::~ScriptEngine()
	{
		delete m_compiler;
		delete m_parser;
		delete m_basicModule;
	}

	// ******************************************************* //

	Module* ScriptEngine::getModule(const string& _name)
	{
		string name = extractName(_name);
		for (auto& mod : m_modules)
		{
			if (mod->m_name == name) return mod.get();
		}
		return loadModule(_name) ? getModule(name) : nullptr;
	}

	// ******************************************************* //

	bool ScriptEngine::loadModule(const string& _fileName, NaReTi::Module* _dest)
	{
		std::ifstream in((m_config.scriptLocation + _fileName).c_str(), std::ios::in | std::ios::binary);

		//check if file is valid
		if (!in)
		{
			cout << "Could not open File " << _fileName;
			return false;
		}

		string fileContent;

		// put filecontent into a string
		in.seekg(0, std::ios::end);
		fileContent.reserve(in.tellg());
		in.seekg(0, std::ios::beg);

		fileContent.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

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
			Module* mod = getModule(modName + ".nrt");
			if (mod) module.m_dependencies.push_back(mod);
			else
			{
				LOG(Warning, "Could not load depended module: " << modName);
		//		break;
			}
		}
		bool ret = m_parser->parse(fileContent, module);
		if (ret)
		{
			if(m_config.optimizationLvl > None) m_optimizer.optimize(module);
			m_compiler->compile(module);
		}
		else
		{
			m_modules.pop_back();
			logging::log(logging::Error, "The module \"" + packageName + "\" could not be loaded due to a parsing error");
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

	bool ScriptEngine::reloadModule(const std::string& _moduleName)
	{
		return unloadModule(_moduleName, false) && loadModule(_moduleName+".nrt");
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