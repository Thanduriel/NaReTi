#pragma once

#include "module.hpp"
#include <asmjit.h>
#include "enginetypes.hpp"

namespace codeGen
{
	struct UsageState
	{
		size_t varsInUse;
		size_t vars32InUse;
		size_t floatsInUse;
	};
	class Compiler
	{
	public:
		Compiler();

		void compile(NaReTi::Module& _module);
		asmjit::JitRuntime& getRuntime() { return m_runtime; }
	private:
		// calculates the member offsets and alignment
		void compileType(par::ComplexType& _type);
		// allocates heap space for a local var and stores the address in _var
		void compileHeapVar(par::VarSymbol& _var, utils::StackAlloc& _allocator);

		// compiles and calls the module init function that contains assignments for global vars
		void compileModuleInit(NaReTi::Module& _module);

		//sets up a par::Function's asmjit funcbuilder
		void convertSignature(par::Function& _function);
		void allocVar(par::VarSymbol& _sym);
		asmjit::X86GpVar* allocStackVar(par::ComplexType& _type, int _count = 1);
		//compile a specific structure
		void compileFuction(par::Function& _function);
		void compileCode(par::ASTCode& _node);
		void compileCall(par::ASTCall& _node, asmjit::Var* _dest = nullptr);
		asmjit::Var* compileLeaf(par::ASTLeaf& _node, bool* _indirect = nullptr);
		void compileOp(par::InstructionType _instr, std::vector< asmjit::Var* >& _args);
		void compileRet(par::ASTReturn& _node);
		void compileRetF(par::ASTReturn& _node);
		
		void compileMemCpy(asmjit::X86GpVar& _dst, asmjit::X86GpVar& _src, size_t _size);
		asmjit::X86Mem getMemberAdr(par::ASTMember& _node);
		//load some member var into the given destination register
		void compileMemberLd(par::ASTMember& _node, asmjit::X86GpVar& _destination);
		void compileMemberLdF(par::ASTMember& _node, asmjit::X86XmmVar& _destination);
		//store result in a member var; not in use
		void compileMemberStr(par::ASTMember& _node);
		void compileBranch(par::ASTBranch& _node);
		void compileLoop(par::ASTLoop& _node);
		//compiles an expression that resolves to a branch decision
		void compileCondExp(par::ASTCall& _node);

		UsageState getUsageState() { return m_usageState; }
		void setUsageState(UsageState& _newState) { m_usageState = _newState; }
		// returns a virtual register currently not in use
		asmjit::X86GpVar& getUnusedVar(); //native size var (x86: 32bit; x64: 64bit)
		asmjit::X86GpVar& getUnusedVar32();
		asmjit::X86XmmVar& getUnusedFloat();
		asmjit::Var* getUnusedVarAuto(par::TypeInfo& _typeInfo); // returns the right var for the given type
		void resetRegisters();



		asmjit::JitRuntime m_runtime;
		asmjit::X86Assembler m_assembler;
		asmjit::X86Compiler m_compiler;

		par::Function* m_function; // currently compiled function
		asmjit::X86GpVar* m_accumulator; //asmjit temp var of the currently compiled function
		asmjit::X86XmmVar* m_fp0;
		//virtual registers used
		std::vector<asmjit::X86GpVar> m_anonymousVars;
		std::vector<asmjit::X86GpVar> m_anonymousVars32;
		std::vector<asmjit::X86XmmVar> m_anonymousFloats;
		// registers that have relevant content that should not be overwritten
		// apart from assignments.
		UsageState m_usageState;

		std::vector< asmjit::Label> m_labelStack; // asm labels required in conditional branches
		bool m_isRefSet; //flag: lvalue is a reference 
		std::vector< asmjit::Var* > m_retDstStack; // return destinations
		// implemented as a pseudo stack to allow recursive inlining
	};
}