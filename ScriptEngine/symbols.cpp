#include "ast.hpp"
#include "symbols.hpp"

namespace par{
	Function::Function(const std::string& _name, Type& _type, InstructionType _instr) : Function(_name, _type)
	{
		bInline = true;
		paramCount = 2;

		//args
		scope.m_variables.emplace_back("0", _type);
		scope.m_variables.emplace_back("1", _type);

		//params do not need to be set
		scope.emplace_back(new ASTBinOp(_instr));
	};
}