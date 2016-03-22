#include "ast.hpp"
#include "symbols.hpp"

namespace par{
	Function::Function(utils::StackAlloc& _alloc, const std::string& _name, ComplexType& _type, InstructionType _instr) : Function(_name, TypeInfo(_type))
	{
		bInline = true;
		bIntrinsic = true;
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
}