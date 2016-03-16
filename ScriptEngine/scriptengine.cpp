#include <fstream>
#include <time.h>  //clock

#include "scriptengine.h"

using namespace std;

namespace NaReTi
{
	ScriptEngine::ScriptEngine()
	{
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

	bool ScriptEngine::loadModule(const string& _fileName)
	{
		std::ifstream in(_fileName.c_str(), std::ios::in | std::ios::binary);

		//check if file is valid
		if (!in)
		{
	//		LOG(ERROR) << "Could not open Code-File " << _fileName;
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
		m_modules.emplace_back(new NaReTi::Module(packageName));
		NaReTi::Module& module = *m_modules.back();

		//look for dependencies
		m_parser.preParse(fileContent, module);
		auto dep = m_parser.getDependencies();
		for (auto& modName : dep)
		{
			Module* mod = getModule(modName + ".nrt");
			if (mod) module.m_dependencies.push_back(mod);
			else
			{
				cout << "Could not load depended module: " << modName << endl;
				break;
			}
		}
		bool ret = m_parser.parse(fileContent, module);
		if (ret)
		{
			m_optimizer.optimize(module);
			m_compiler.compile(module);
		}
		else
		{
			m_modules.pop_back();
		}

		return ret;
	}

	// ******************************************************* //

	bool ScriptEngine::unloadModule(const std::string& _moduleName)
	{
		for (auto i = m_modules.begin(); i != m_modules.end(); ++i)
		{
			Module& module = *(*i);
			if (module.m_name == _moduleName)
			{
				m_modules.erase(i);
				return true;
			}
		}
		return false;
	}

	// ******************************************************* //

	FunctionHandle ScriptEngine::getFuncHndl(const std::string& _name)
	{
		for (auto& module : m_modules)
			for (auto& func : module->m_functions)
				if (func->name == _name) return FunctionHandle(func->binary);

		return FunctionHandle();
	}

	// ******************************************************* //

	void ScriptEngine::call(FunctionHandle _hndl)
	{
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