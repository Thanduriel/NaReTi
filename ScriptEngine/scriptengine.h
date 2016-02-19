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

		//compiles a given source file and adds the module
		bool loadModule(const std::string& _fileName);


		//
		FunctionHandle getFuncHndl(const std::string& _name);

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
		tok::Lexer m_lexer;
		par::Parser m_parser;
		codeGen::Compiler m_compiler;

		std::vector< FunctionHandle > m_nativeFunctions;
		std::vector< std::unique_ptr<Module> > m_modules;
	};

}
