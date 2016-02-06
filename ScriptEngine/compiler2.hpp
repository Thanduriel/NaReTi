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
		void translateInstruction(par::Instruction& _instruction);

		// inlines the given call instr(found at instructions[id]) in the current function context
		// expanding the original code scope
		// returns the id to the first instruction after the inlined call
		size_t inlineFunction(size_t _id);

		asmjit::JitRuntime m_runtime;
		asmjit::X86Assembler m_assembler;
		asmjit::X86Compiler m_compiler;

		par::Function* m_function; // currently compiled function
		asmjit::X86GpVar m_accumulator; //asmjit temp var of the currently compiled function
	};
}