#include "module.hpp"
#include "atomics.hpp"
#include "symbols.hpp"
#include "generics.hpp"
#include "ast.hpp"

namespace NaReTi{
	using namespace par;
	using namespace std;

	Module::Module(const std::string& _name) 
		: m_name(_name),
		m_text(new ASTCode())
	{

	}

	Module::~Module()
	{
		delete m_text;
		for (auto el : m_types) delete el;
		for (auto el : m_genericTypes) delete el;
		for (auto el : m_functions) delete el;
	}

	par::ComplexType* Module::getType(const std::string& _name)
	{
		for (auto& type : m_types)
			if (type->name == _name) return type;

		for (auto& alias : m_typeAlias)
			if (alias.first == _name) return alias.second;

		return nullptr;
	}

	TypeAlias* Module::getTypeAlias(const std::string& _name)
	{
		for (auto& alias : m_typeAlias)
			if (alias.first == _name) return &alias;

		return nullptr;
	}

	par::Function* Module::getFunction(const std::string& _name,
		const std::vector<par::ASTExpNode*>::iterator& _begin,
		const std::vector<par::ASTExpNode*>::iterator& _end,
		std::vector<FuncMatch>& _funcQuery)
	{
		for (auto& func : m_functions)
		{
			if (func->name != _name) continue;
	//		if (&func.returnType != &_ret) continue;

			int i = func->bHiddenParam ? 1 : 0;
			auto dist = std::distance(_begin, _end);
			if (dist != func->paramCount-i) continue;

			_funcQuery.emplace_back(*func);
			FuncMatch& match = _funcQuery.back();
			auto begin = _begin;
			// hidden param is not part of the signature
			for (; i < func->paramCount; ++i)
			{
				par::ASTExpNode* found = *(begin++);

				//type does not match
				if (*found->typeInfo  != func->scope.m_variables[i]->typeInfo)
				{
					//casts with data loss are of smaller priority
					if ((*found->typeInfo).type.basic == BasicType::Float && func->scope.m_variables[i]->typeInfo.type.basic == BasicType::Int) 
						match.diff++;
					match.diff += 64; // leave enough space
				}
			}
			if (!match.diff) return func;
		}
		return nullptr;
	}

	Function* Module::getFunction(const std::string& _name, bool _external)
	{
		auto it = std::find_if(m_functions.begin(), m_functions.end(),
			[&](const Function* _f)
		{ 
			return _f->name == _name && (!_external || _f->bExternal == _external); 
		});

		return it == m_functions.end() ? nullptr : (*it);
	/*	for (auto& func : m_functions)
		{
			if (func->name == _name && (!_external || func->bExternal == _external)) return func.get();
		}
		return nullptr;*/
	}

	VarSymbol* Module::getGlobalVar(const std::string& _name)
	{
		auto it = std::find_if(m_text->m_variables.begin(), m_text->m_variables.end(),
			[&](const VarSymbol* _v)
		{
			return _v->name == _name;
		});

		return it == m_text->m_variables.end() ? nullptr : (*it);

		/*
		for (auto& var : m_text->m_variables)
		{
			if (_name == var->name) return var;
		}*/
		return nullptr;
	}

	// ******************************************************** //

	bool Module::linkExternal(const string& _name, void* _funcPtr)
	{
		Function* func = getFunction(_name, true);
		if (!func) return false;

		func->binary = _funcPtr;
		return true;
	}
}