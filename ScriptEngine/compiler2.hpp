#pragma once

#include "module.hpp"
#include <asmjit.h>

namespace codeGen
{
	class Compiler
	{
	public:
		Compiler();

		void compile(NaReTi::Module& _module);

	private:
		void compileFuction(par::Function& _function);
		void compileCode(par::ASTCode& _node);
		void compileCall(par::ASTCall& _node, size_t _anonUsed = 0);
		void compileBinOp(par::ASTBinOp& _node);
		void compileRet(par::ASTReturn& _node);
	//	void translateNode(par::ASTNode& _node);

		asmjit::JitRuntime m_runtime;
		asmjit::X86Assembler m_assembler;
		asmjit::X86Compiler m_compiler;

		par::Function* m_function; // currently compiled function
		asmjit::X86GpVar m_accumulator; //asmjit temp var of the currently compiled function
		std::vector<asmjit::X86GpVar> m_anonymousVars; //additional temp vars for function inlining
	};
}