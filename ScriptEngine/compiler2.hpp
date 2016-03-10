#pragma once

#include "module.hpp"
#include <asmjit.h>

namespace codeGen
{
	struct UsageState
	{
		size_t varsInUse;
		size_t floatsInUse;
	};
	class Compiler
	{
	public:
		Compiler();

		void compile(NaReTi::Module& _module);

	private:
		// calculates the member offsets and alignment
		void compileType(par::ComplexType& _type);

		void compileHeapVar(par::VarSymbol& _var, utils::StackAlloc& _allocator);

		//sets up a par::Function's asmjit funcbuilder
		void convertSignature(par::Function& _function);
		//compile a specific structure
		void compileFuction(par::Function& _function);
		void compileCode(par::ASTCode& _node);
		void compileCall(par::ASTCall& _node);
		asmjit::Operand* compileLeaf(par::ASTLeaf& _node);
		void compileOp(par::InstructionType _instr, std::vector< asmjit::Operand* >& _args);
		void compileRet(par::ASTReturn& _node);
		void compileRetF(par::ASTReturn& _node);
		asmjit::X86Mem getMemberAdr(par::ASTMember& _node);
		//load some member var into the given destination register
		void compileMemberLd(par::ASTMember& _node, asmjit::Operand* _destination);
		//store result in a member var
		void compileMemberStr(par::ASTMember& _node);
		void compileBranch(par::ASTBranch& _node);
		void compileLoop(par::ASTLoop& _node);
		//compiles an expression that resolves to a branch descicion
		void compileCondExp(par::ASTCall& _node);

		UsageState getUsageState() { return m_usageState; }
		void setUsageState(UsageState& _newState) { m_usageState = _newState; }
		// returns a virtual register currently not in use
		asmjit::X86GpVar& getUnusedVar();
		asmjit::X86XmmVar& getUnusedFloat();



		asmjit::JitRuntime m_runtime;
		asmjit::X86Assembler m_assembler;
		asmjit::X86Compiler m_compiler;

		par::Function* m_function; // currently compiled function
		asmjit::X86GpVar* m_accumulator; //asmjit temp var of the currently compiled function
		asmjit::X86XmmVar* m_fp0;
		//virtual registers used
		std::vector<asmjit::X86GpVar> m_anonymousVars; 
		std::vector<asmjit::X86XmmVar> m_anonymousFloats;
		// registers that have relevant content that should not be overwritten
		// apart from assignments.
		UsageState m_usageState;

		std::vector< asmjit::Label> m_labelStack; // asm labels required in conditional branches
		bool m_isRefSet; //flag: lvalue is a reference 
	};
}