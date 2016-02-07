#pragma once
#include <vector>
#include "symbols.hpp"

namespace par{

	
	// ************************************* //
	// abstract syntax tree
	// ************************************* //

	//Any node has to be casted to its specific type
	enum class ASTType
	{
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
		Function* function;
		std::vector< ASTNode* > args;
	};

	struct ASTCode : public ASTNode
	{
		std::vector< ASTNode* > code;
	};

	class ASTBranch : public ASTNode
	{
		ASTCall* condition;
		ASTCode* ifBody;
		ASTCode* elseBody;
	};

	class ASTLoop : public ASTNode
	{
		ASTCall* condition;
		ASTCode* body;
	};

	class ASLeaf : public ASTNode
	{

	};
}