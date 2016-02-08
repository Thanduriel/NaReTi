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
		Ret
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
	};
}