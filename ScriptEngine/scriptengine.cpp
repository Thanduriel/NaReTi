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

	bool ScriptEngine::loadMdoule(const string& _fileName)
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

		//extract file name
		size_t endInd = _fileName.size()-1;
		size_t beginInd = 0;
		size_t i = _fileName.size();
		while (i != 0)
		{
			i--;
			if (_fileName[i] == '.') endInd = i;
			else if (_fileName[i] == '\\' || _fileName[i] == '/') 
			{
				beginInd = i+1;
				break;
			}
		}
		//use it witout the file ending as name for the module
		string packageName = _fileName.substr(beginInd, endInd);
		NaReTi::Module module(packageName);


		bool ret = m_parser.parse(fileContent, module);
		if (ret)
		{
			m_compiler.compile(module);
			m_modules.push_back(std::move(module));
		}

		return ret;
	}

	// ******************************************************* //

	FunctionHandle ScriptEngine::getFuncHndl(const std::string& _name)
	{
		for (auto& module : m_modules)
			for (auto& func : module.m_functions)
				if (func.name == _name) return FunctionHandle(func.binary);
	}

	// ******************************************************* //

	void ScriptEngine::call(FunctionHandle _hndl)
	{
		((basicFunc*)_hndl.ptr)();
	}
}