#include "module.hpp"
#include "atomics.hpp"

namespace NaReTi
{
	using namespace par;

	par::ComplexType* Module::getType(const std::string& _name)
	{
		for (auto& type : m_types)
			if (type->name == _name) return type.get();

		return nullptr;
	}

	par::Function* Module::getFunction(const std::string& _name,
		const std::vector<par::ASTExpNode*>::iterator& _begin,
		const std::vector<par::ASTExpNode*>::iterator& _end)
	{
		for (auto& func : m_functions)
		{
			if (func->name != _name) continue;
	//		if (&func.returnType != &_ret) continue;

			auto dist = std::distance(_begin, _end);
			if (dist != func->paramCount) continue;

			//make a copy
			auto begin = _begin;
			bool paramsMatch = true;
			for (int i = 0; i < dist; ++i)
			{
				par::ASTExpNode* found = *(_begin + i);

				if (*found->typeInfo != func->scope.m_variables[i].typeInfo)
				{
					paramsMatch = false;
					break;
				}
			}
			if (paramsMatch) return func.get();
		}
		return nullptr;
	}

	VarSymbol* Module::getGlobalVar(const std::string& _name)
	{
		for (auto& var : m_text.m_variables)
		{
			if (_name == var.name) return &var;
		}
		return nullptr;
	}
}