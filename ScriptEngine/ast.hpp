#pragma once
#include <vector>
#include <memory>

#include "symbols.hpp"
#include "instruction.hpp"
#include "stackalloc.hpp"

namespace par{

	
	// ************************************* //
	// abstract syntax tree
	// ************************************* //

	//Any node has to be casted to its specific type
	enum class ASTType
	{
		Code,
		Branch,
		Call,
		Leaf,
		Loop,
		Ret,
		BinOp,
		Member
	};

	struct ASTNode
	{
		ASTType type;
	};

	// node with a type
	struct ASTExpNode : public ASTNode
	{
		TypeInfo* typeInfo;
	};

	struct ASTCall : public ASTExpNode
	{
		ASTCall(): isLocked(false) { type = ASTType::Call; }
		Function* function;
		std::vector< ASTExpNode* > args;

		bool isLocked; // a parsing flag, meaning that this subtree may not be changed
	};

	struct ASTCode : public ASTNode, 
		public CodeScope, 
		public std::vector< ASTNode* >
	{
		ASTCode() { type = ASTType::Code; }
	};

	//function symbol; just here because of crosslinks between ast and symbols
	struct Function : public Symbol, codeGen::CFunction
	{
		//definitions can be fuond in "symbols.cpp"
		// a binary function of the structure T x T -> T
		Function(utils::StackAlloc& _alloc, const std::string& _name, ComplexType& _type, InstructionType _instr);
		// general purpose T1 x T2 -> T0
		Function(utils::StackAlloc& _alloc, const std::string& _name, std::initializer_list<InstructionType> _instr, ComplexType& _t0, ComplexType* _t1, ComplexType* _t2);
		Function(const std::string& _name, ComplexType& _type) : Symbol(_name), returnTypeInfo(_type, false){};

		TypeInfo returnTypeInfo;
		ASTCode scope;
		int paramCount; //< amount of params this function expects, coresponds to the first elements in scope.locals
		//flags
		bool bInline;
	};

	struct ASTBranch : public ASTNode
	{
		ASTBranch() { type = ASTType::Branch; }

		ASTNode* condition;
		ASTCode* ifBody;
		ASTCode* elseBody;
	};

	struct ASTLoop : public ASTNode
	{
		ASTLoop() { type = ASTType::Loop; }

		ASTNode* condition;
		ASTCode* body;
	};

	struct ASTReturn : public ASTNode
	{
		ASTReturn() { type = ASTType::Ret; }

		ASTExpNode* body;
	};

	struct ASTLeaf : public ASTExpNode, public Parameter
	{
		ASTLeaf() { type = ASTType::Leaf; }
		ASTLeaf(VarSymbol* _val) : Parameter(_val){ type = ASTType::Leaf; }
		ASTLeaf(int _val) : Parameter(_val){ type = ASTType::Leaf; }
	};

	struct ASTOp : public ASTNode
	{
		ASTOp(InstructionType _instr) : instruction(_instr) { type = ASTType::BinOp; }
		InstructionType instruction;

//		Type* returnType; //setting the type is not mandentory in inlined functions
	};

	struct ASTMember : public ASTExpNode
	{
		ASTMember(ASTExpNode& _var, size_t _index) : instance(&_var), index(_index) { type = ASTType::Member; }
		
		ASTExpNode* instance;
		size_t index;
	};
}