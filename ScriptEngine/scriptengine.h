#include "lexer.hpp"
#include "enginetypes.hpp"
#include "parser.hpp"
#include "compiler2.hpp"

#pragma once

namespace NaReTi{

	class ScriptEngine
	{
	public:
		ScriptEngine();

		// Loads the module if it is not found.
		Module* getModule(const std::string& _name);
		//compiles a given source file and adds the module
		//the name is the file name without ending.
		bool loadModule(const std::string& _fileName);
		// Removes a module with the given name and all it's symbols to release it's memory.
		// This should only be used if no other module depends on the module.
		bool unloadModule(const std::string& _moduleName);

		//Creates a new and empty module to build extern.
		Module& createModule(const std::string& _moduleName);

		//returns a handle to the function with the given name if existent
		FunctionHandle getFuncHndl(const std::string& _name);

		//call without template params(just for convenience)
		void call(FunctionHandle _hndl);
		
		template< typename _Ret, typename... _Args>
		_Ret call(FunctionHandle _hndl, _Args... _args)
		{
			//magic typecasting to the given function type
			auto func = (_Ret (*) (_Args...))_hndl.ptr;
			//the call
			return func(_args...);
		};

	private:
		std::string extractName(const std::string& _fullName);
		par::Parser m_parser;
		codeGen::Compiler m_compiler;

		std::vector< FunctionHandle > m_nativeFunctions;
		std::vector< std::unique_ptr<Module> > m_modules;
	};

}
