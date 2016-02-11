#pragma once
#include <vector>
#include <memory>

#include "symbols.hpp"
#include "instruction.hpp"

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
		BinOp
	};

	struct ASTNode
	{
		ASTType type;
	};

	struct ASTCall : public ASTNode
	{
		ASTCall() { type = ASTType::Call; }
		Function* function;
		std::vector< ASTNode* > args;
	};

	struct ASTCode : public ASTNode, 
		public CodeScope, 
		public std::vector< ASTNode* >
	{
		ASTCode() { type = ASTType::Code; }
	};

	//function symbol; just here because of crosslinks between ast and symbols
	struct Function : public Symbol
	{
		// a binary function of the structure T x T -> T
		Function(const std::string& _name, Type& _type, InstructionType _instr);
		Function(const std::string& _name, Type& _type) : Symbol(_name), returnType(_type){};

		Type& returnType;
		ASTCode scope;
		int paramCount; //< amount of params this function expects, coresponds to the first elements in scope.locals
		//flags
		bool bInline;

		//the compiled version (the callee should know its signiture)
		void* binary;
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

		ASTNode* body;
	};

	struct ASTLeaf : public ASTNode, public Parameter
	{
		ASTLeaf() { type = ASTType::Leaf; }
		ASTLeaf(VarSymbol* _val) : Parameter(_val){ type = ASTType::Leaf; }
		ASTLeaf(int _val) : Parameter(_val){ type = ASTType::Leaf; }
	};

	struct ASTBinOp : public ASTNode
	{
		ASTBinOp(InstructionType _instr) : instruction(_instr) { type = ASTType::BinOp; }
		InstructionType instruction;

		Type* returnType; //setting the type is not mandentory in inlined functions
		//left and right operand
		ASTLeaf* lOperand;
		ASTLeaf* rOperand;
	};
}