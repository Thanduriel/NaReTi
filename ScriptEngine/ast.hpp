#pragma once
#include <vector>
#include <memory>

#include "symbols.hpp"
#include "instruction.hpp"
#include "complexalloc.hpp"

namespace par{

	
	// ************************************* //
	// abstract syntax tree
	// ************************************* //

	//Any node has to be casted to its specific type
	
	enum struct ASTType
	{
		Code,
		Branch,
		Call,
		Loop,
		Ret,
		BinOp,
		Member,
		String,
		Leaf,
		LeafInt,
		LeafSym,
		LeafFloat,
		LeafSizeOf,
		LeafAddress
	};

	struct ASTNode : public utils::DetorAlloc::Destructible
	{
		ASTType type;
	};

	// node with a type
	struct ASTExpNode : public ASTNode
	{
		const TypeInfo* typeInfo;
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

	typedef std::vector< ASTNode* > Text;

	struct ASTCode : public ASTNode, 
		public CodeScope, 
		public Text
	{
		ASTCode() : parent(0) { type = ASTType::Code; }
		ASTCode* parent;
		Text epilogue; // destructor calls
	};

	//function symbol; just here because of cross-links between ast and symbols
	struct Function : public Symbol, codeGen::CFunction
	{
		//definitions can be found in "symbols.cpp"
		// a binary function of the structure T x T -> T
		Function(utils::DetorAlloc& _alloc, const std::string& _name, ComplexType& _type, InstructionType _instr);
		// general purpose T1 x T2 -> T0
		Function(utils::DetorAlloc& _alloc, const std::string& _name, std::initializer_list<InstructionType> _instr, ComplexType& _t0, ComplexType* _t1, ComplexType* _t2);
		//typecast
		Function(utils::DetorAlloc& _alloc, const std::string& _name, InstructionType _instr, const TypeInfo& _t0, const TypeInfo& _t1);
		//standard
		Function(const std::string& _name, const TypeInfo& _type) : Symbol(_name), returnTypeInfo(_type), bExternal(false), bHiddenParam(false), bInline(false), bIntrinsic(false){};

		TypeInfo returnTypeInfo;
		ASTCode scope;
		int paramCount; //< amount of params this function expects, corresponds to the first elements in scope.locals
		//flags
		bool bInline;
		bool bIntrinsic; // is constructed from instructions
		bool bExternal;
		bool bHiddenParam; // has a hidden first parameter

		enum IntrinsicType
		{
			Assignment,
			BinOp,
			Compare,
			TypeCast,
			StaticCast // do nothing, just return the arg as destination
		} intrinsicType;
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

		ASTExpNode* condition;
		ASTCode* body;
	};

	struct ASTReturn : public ASTNode
	{
		ASTReturn() { type = ASTType::Ret; }

		ASTExpNode* body;
	};

	//just additional typesafety
	struct ASTLeaf : public ASTExpNode
	{};

	template < typename _T, ASTType _astType>
	struct ASTLeafT : public ASTExpNode
	{
		ASTLeafT(_T _value) : value(_value)
		{ 
			static_assert((int)_astType > (int)ASTType::Leaf, "Leaf types have to be at the end of the type enum.");
			type = _astType;
		}		
		
		_T value;
	};

	typedef ASTLeafT<int, ASTType::LeafInt> ASTLeafInt;
	typedef ASTLeafT<float, ASTType::LeafFloat> ASTLeafFloat;
	typedef ASTLeafT<VarSymbol*, ASTType::LeafSym> ASTLeafSym;
	typedef ASTLeafT<TypeInfo, ASTType::LeafSizeOf> ASTSizeOf;
	typedef ASTLeafT<uint64_t, ASTType::LeafAddress> ASTLeafAdr;

	struct ASTUnlinkedSym : public ASTExpNode
	{
		ASTUnlinkedSym(const std::string& _name) : name(_name){ type = ASTType::String; }
		std::string name;
	};

	struct ASTOp : public ASTNode
	{
		ASTOp(InstructionType _instr) : instruction(_instr) { type = ASTType::BinOp; }
		InstructionType instruction;
	};

	struct ASTMember : public ASTCall
	{
		ASTMember() { type = ASTType::Member; }
		
		ASTExpNode* instance;
		size_t index;
	};
}