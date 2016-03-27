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
		Member,
		String
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
		ASTCall(): isLocked(false), returnSub(nullptr) { type = ASTType::Call; }
		Function* function;
		std::vector< ASTExpNode* > args;

		std::string name; // signatures can only be checked once a complete expression has been parsed
		bool isLocked; // a parsing flag, meaning that this subtree may not be changed
		VarSymbol* returnSub;
	};

	struct ASTCode : public ASTNode, 
		public CodeScope, 
		public std::vector< ASTNode* >
	{
		ASTCode() { type = ASTType::Code; }
		ASTCode* parent;
	};

	//function symbol; just here because of crosslinks between ast and symbols
	struct Function : public Symbol, codeGen::CFunction
	{
		//definitions can be found in "symbols.cpp"
		// a binary function of the structure T x T -> T
		Function(utils::StackAlloc& _alloc, const std::string& _name, ComplexType& _type, InstructionType _instr);
		// general purpose T1 x T2 -> T0
		Function(utils::StackAlloc& _alloc, const std::string& _name, std::initializer_list<InstructionType> _instr, ComplexType& _t0, ComplexType* _t1, ComplexType* _t2);
		Function(const std::string& _name, TypeInfo& _type) : Symbol(_name), returnTypeInfo(_type), bExternal(false), bHiddenParam(false), bInline(false), bIntrinsic(false){};

		TypeInfo returnTypeInfo;
		ASTCode scope;
		int paramCount; //< amount of params this function expects, corresponds to the first elements in scope.locals
		//flags
		bool bInline;
		bool bIntrinsic; // is constructed from instructions
		bool bExternal;
		bool bHiddenParam; // has a hidden first parameter
	};

	struct ASTBranch : public ASTNode
	{
		ASTBranch() : elseBody(nullptr) { type = ASTType::Branch; }

		ASTExpNode* condition;
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
		ASTLeaf(float _val) : Parameter(_val){ type = ASTType::Leaf; }
	};

	struct ASTUnlinkedSym : public ASTExpNode
	{
		ASTUnlinkedSym(const std::string& _name) : name(_name){ type = ASTType::String; }
		std::string name;
	};

	struct ASTOp : public ASTNode
	{
		ASTOp(InstructionType _instr) : instruction(_instr) { type = ASTType::BinOp; }
		InstructionType instruction;

//		Type* returnType; //setting the type is not mandatory in inlined functions
	};

	struct ASTMember : public ASTCall
	{
		ASTMember() { type = ASTType::Member; }
		
		ASTExpNode* instance;
		size_t index;
	//	ASTUnlinkedSym* member;
	};
}