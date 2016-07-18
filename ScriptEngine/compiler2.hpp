#pragma once

#include "module.hpp"
#include <asmjit.h>
#include <array>
#include "enginetypes.hpp"
#include "ast.hpp"
#include "ptr_stuff.hpp"

namespace codeGen
{
	struct UsageState
	{
		size_t varsInUse;
		size_t vars32InUse;
		size_t floatsInUse;
	};

	struct UsageStateLock
	{
		UsageStateLock(UsageState& _state) : 
			state(_state), target(_state){}
		~UsageStateLock(){ target = state; }
		void reset(){ state = target; }

		UsageState state;
		UsageState& target;
	};
	class Compiler
	{
	public:
		Compiler();

		void compile(NaReTi::Module& _module);
		//releases all binary data owned by the symbol: functions, global vars
		void release(NaReTi::Module& _module);
		asmjit::JitRuntime& getRuntime() { return m_runtime; }
	private:
		// calculates the member offsets and alignment
		void compileType(par::ComplexType& _type);
		// allocates heap space for a local var and stores the address in _var
		void compileHeapVar(par::VarSymbol& _var);

		// compiles and calls the module init function that contains assignments for global vars
		void compileModuleInit(NaReTi::Module& _module);

		//sets up a par::Function's asmjit funcbuilder
		void convertSignature(par::Function& _function);
		void allocVar(par::VarSymbol& _sym);
		asmjit::X86GpVar* allocStackVar(par::ComplexType& _type, int _count = 1);
		//compile a specific structure
		void compileFuction(par::Function& _function);
		//@param _preAllocCount local variables that are already allocated
		void compileCode(par::ASTCode& _node, int _preAllocCount = 0);
		//@param _dest where the result should be stored
		//@return the destination of the return
		asmjit::Var* compileCall(par::ASTCall& _node, bool _keepRet = true, asmjit::Var* _dest = nullptr);
		asmjit::Var* compileLeaf(par::ASTLeaf& _node, bool* _indirect = nullptr);
		void compileOp(par::InstructionType _instr, std::vector< asmjit::Var* >& _args, bool _indirect);
		void compileRet(par::ASTReturn& _node);
		void compileRetF(par::ASTReturn& _node);
		
		void compileMemCpy(asmjit::X86GpVar& _dst, asmjit::X86GpVar& _src, size_t _size);
		// returns the var the address of the instance is stored in
		// in a nested member access the instance of the last member is given
		asmjit::X86GpVar* compileMemberAdr(par::ASTMember& _node); 
		asmjit::X86Mem getMemberAdr(par::ASTMember& _node, asmjit::X86GpVar& _var);
		//load some member var into the given destination register
		void compileMemberLd(par::ASTMember& _node, asmjit::X86GpVar& _var, asmjit::X86GpVar& _destination);
		void compileMemberLdF(par::ASTMember& _node, asmjit::X86GpVar& _var,  asmjit::X86XmmVar& _destination);
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
		bool m_arg0IsPtr; //flag: lvalue is a reference 
		std::vector< asmjit::Var* > m_retDstStack; // return destinations
		// implemented as a pseudo stack to allow recursive inlining
	};
}