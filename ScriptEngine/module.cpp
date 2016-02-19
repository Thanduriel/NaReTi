#include "module.hpp"
#include "atomics.hpp"

namespace NaReTi
{
	par::Type* Module::getType(const std::string& _name)
	{
		for (auto& type : m_types)
			if (type->name == _name) return type.get();

		return nullptr;
	}

	par::Function* Module::getFunction(const std::string& _name,
		const std::vector<par::ASTNode*>::iterator& _begin,
		const std::vector<par::ASTNode*>::iterator& _end)
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
				par::Type* foundType;

				par::ASTNode* found = *(_begin + i);
				if (found->type == par::ASTType::Leaf)
				{
					par::ASTLeaf& leaf = *(par::ASTLeaf*)found;
					switch (leaf.parType)
					{
					case par::ParamType::Ptr: foundType = &leaf.ptr->type; break;
					case par::ParamType::Int: foundType = &lang::g_module.getBasicType(par::BasicType::Int);
					}
				}
				else if (found->type == par::ASTType::Call) foundType = &((par::ASTCall*)found)->function->returnType;
				else if (found->type == par::ASTType::BinOp) foundType = ((par::ASTBinOp*)found)->returnType;
				//todo atomic types can be valid aswell
				else
				{
					paramsMatch = false;
					break;
				}
				if (foundType != &func->scope.m_variables[i].type)
				{
					paramsMatch = false;
					break;
				}
			}
			if (paramsMatch) return func.get();
		}
		return nullptr;
	}
}