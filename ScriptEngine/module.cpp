#include "module.hpp"
#include "atomics.hpp"

namespace NaReTi
{
	using namespace par;
	using namespace std;

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

			int i = func->bHiddenParam ? 1 : 0;
			auto dist = std::distance(_begin, _end);
			if (dist != func->paramCount-i) continue;

			bool paramsMatch = true;
			auto begin = _begin;
			// hidden param is not part of the signature
			for (; i < dist; ++i)
			{
				par::ASTExpNode* found = *(begin++);

				//type does not match
				if (&found->typeInfo->type != &func->scope.m_variables[i].typeInfo.type)
				{
					paramsMatch = false;
					break;
				}
				//reference or value does not match
				// complex types are always by reference and the distinction only matters for the caller
				else if (!(found->typeInfo->type.basic == BasicType::Complex
					|| found->typeInfo->isReference == func->scope.m_variables[i].typeInfo.isReference))
				{
					paramsMatch = false;
					break;
				}
			}
			if (paramsMatch) return func.get();
		}
		return nullptr;
	}

	Function* Module::getFunction(const std::string& _name)
	{
		for (auto& func : m_functions)
		{
			if (func->name == _name) return func.get();
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

	// ******************************************************** //

	bool Module::linkExternal(const string& _name, void* _funcPtr)
	{
		Function* func = getFunction(_name);
		if (!func) return false;

		func->binary = _funcPtr;
		return true;
	}
}