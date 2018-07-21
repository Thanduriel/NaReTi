#pragma once

// this file is named compiler2.hpp to prevent a name collision with asmjit

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
		void convertSignature(par::Function& _function) const;
		void allocVar(par::VarSymbol& _sym);
		asmjit::X86Gp* allocStackVar(const par::ComplexType& _type, int _count = 1);
		//compile a specific structure
		void compileFuction(par::Function& _function);
		//@param _preAllocCount local variables that are already allocated
		void compileCode(par::ASTCode& _node, int _preAllocCount = 0);
		void compileEpilogue(const par::ASTCode& _node);
		//@param _dest where the result should be stored
		//@return the destination of the return
		asmjit::Operand* compileCall(par::ASTCall& _node, bool _keepRet = true, asmjit::Operand* _dest = nullptr);
		asmjit::Operand* compileLeaf(par::ASTLeaf& _node, int* _indirect = nullptr);
		void compileOp(par::InstructionType _instr, const std::vector< asmjit::Operand* >& _args, int _indirect);
		void compileRet(const par::ASTReturn& _node);
		void compileRetF(const par::ASTReturn& _node);
		
		void compileMemCpy(const asmjit::X86Gp& _dst, const asmjit::X86Gp& _src, size_t _size);
		// returns the var the address of the instance is stored in
		// in a nested member access the instance of the last member is given
		asmjit::X86Gp* compileMemberAdr(par::ASTMember& _node); 
		asmjit::X86Mem getMemberAdr(par::ASTMember& _node, asmjit::X86Gp& _var);
		//load some member var into the given destination register
		void compileMemberLd(par::ASTMember& _node, asmjit::X86Gp& _var, asmjit::X86Gp& _destination);
		void compileMemberLdF(par::ASTMember& _node, asmjit::X86Gp& _var,  asmjit::X86Xmm& _destination);
		//store result in a member var; not in use
		void compileMemberStr(par::ASTMember& _node);
		void compileBranch(par::ASTBranch& _node);
		void compileLoop(par::ASTLoop& _node);
		//compiles an expression that resolves to a branch decision
		void compileCondExp(par::ASTCall& _node);

		UsageState getUsageState() const { return m_usageState; }
		void setUsageState(const UsageState& _newState) { m_usageState = _newState; }

		par::ASTCode* m_currentCodeNode;
		// returns a virtual register currently not in use
		asmjit::X86Gp& getUnusedVar(); //native size var (x86: 32bit; x64: 64bit)
		asmjit::X86Gp& getUnusedVar32();
		asmjit::X86Xmm& getUnusedFloat();
		asmjit::Reg* getUnusedVarAuto(const par::TypeInfo& _typeInfo); // returns the right var for the given type
		void resetRegisters();

		asmjit::JitRuntime m_runtime;
		asmjit::CodeHolder m_codeHolder;
	//	asmjit::X86Assembler m_assembler;
		asmjit::X86Compiler m_compiler;

		par::Function* m_function; // currently compiled function
		asmjit::X86Gp* m_accumulator; //asmjit temp var of the currently compiled function

		//virtual registers used
		std::vector<asmjit::X86Gp> m_anonymousVars;
		std::vector<asmjit::X86Gp> m_anonymousVars32;
		std::vector<asmjit::X86Xmm> m_anonymousFloats;
		// registers that have relevant content that should not be overwritten
		// apart from assignments.
		UsageState m_usageState;

		std::vector< asmjit::Label> m_labelStack; // asm labels required in conditional branches
		std::vector< std::pair<asmjit::Operand*, asmjit::Label> > m_retDstStack; // return destinations for inlining
	};
}