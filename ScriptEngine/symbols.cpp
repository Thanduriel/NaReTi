#include "ast.hpp"
#include "symbols.hpp"

namespace par{
	Function::Function(utils::StackAlloc& _alloc, const std::string& _name, ComplexType& _type, InstructionType _instr) : Function(_name, TypeInfo(_type))
	{
		bInline = true;
		bIntrinsic = true;
		paramCount = 2;

		//args
		scope.m_variables.emplace_back("0", _type);
		scope.m_variables.emplace_back("1", _type);

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
		scope.m_variables.emplace_back("0", *_t1);
		scope.m_variables.emplace_back("1", *_t2);

		//params do not need to be set
		for (auto instr : _instr)
			scope.emplace_back(_alloc.construct<ASTOp>(instr));
	};
}