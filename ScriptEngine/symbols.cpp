#include "ast.hpp"
#include "symbols.hpp"

namespace par{
	Function::Function(utils::StackAlloc& _alloc, const std::string& _name, ComplexType& _type, InstructionType _instr) : Function(_name, TypeInfo(_type))
	{
		bInline = true;
		bIntrinsic = true;
		intrinsicType = Function::BinOp;
		paramCount = 2;

		//args
		TypeInfo typeInfo(_type, false, true);
		scope.m_variables.push_back(_alloc.construct<VarSymbol>("0", typeInfo));
		scope.m_variables.push_back(_alloc.construct<VarSymbol>("1", typeInfo));

		//params do not need to be set
		scope.emplace_back(_alloc.construct<ASTOp>(_instr));
	};

	Function::Function(utils::StackAlloc& _alloc, const std::string& _name, std::initializer_list<InstructionType> _instr, ComplexType& _t0, ComplexType* _t1, ComplexType* _t2) : Function(_name, TypeInfo(_t0))
	{
		bInline = true;
		bIntrinsic = true;
		//FlagBool should only be returned by comparison
		intrinsicType = _t0.basic == BasicType::FlagBool ? Function::Compare : Function::BinOp;
		paramCount = 2;

		//args
		if (!_t1) _t1 = &_t0;
		if (!_t2) _t2 = &_t0;
		scope.m_variables.push_back(_alloc.construct<VarSymbol>("0", TypeInfo(*_t1, false, true)));
		scope.m_variables.push_back(_alloc.construct<VarSymbol>("1", TypeInfo(*_t2, false, true)));

		//params do not need to be set
		for (auto instr : _instr)
			scope.emplace_back(_alloc.construct<ASTOp>(instr));
	};

	Function::Function(utils::StackAlloc& _alloc, const std::string& _name, InstructionType _instr, TypeInfo& _t0, TypeInfo& _t1)
		: Function(_name, TypeInfo(_t0))
	{
		bInline = true;
		bIntrinsic = true;
		intrinsicType = Function::TypeCast;
		paramCount = 1;

		scope.m_variables.push_back(_alloc.construct<VarSymbol>("0", _t1));
		scope.emplace_back(_alloc.construct<ASTOp>(_instr));
	}

	// ************************************************ //
	
	bool TypeInfo::operator==(TypeInfo& oth)
	{
		return &type == &oth.type
			&& (type.basic == BasicType::Complex || isArray || isReference == oth.isReference)
			&& !(isConst && !oth.isConst)
			&& isArray == oth.isArray;
	}

	bool TypeInfo::operator!= (TypeInfo& oth)
	{
		return !(*this == oth);
/*		// complex types are always by reference and the distinction only matters for the caller
		return &type != &oth.type
			|| !(type.basic == BasicType::Complex
			|| isReference == oth.isReference)
			|| (isConst && !oth.isConst)
			|| isArray != oth.isArray;*/
	}
}