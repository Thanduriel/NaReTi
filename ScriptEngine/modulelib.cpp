#include "modulelib.hpp"

//#define ALLSUBMODULES (name) for (auto& module : m_modules){ Type* type = module->getType(_name); if (type) return type; } return nullptr;


namespace par
{
	ComplexType* ModuleLibrary::getType(const std::string& _name)
	{
		for (auto& module : m_modules)
		{
			ComplexType* type = module->getType(_name);
			if (type) return type;
		}
		return nullptr;
	}

	par::Function* ModuleLibrary::getFunction(const std::string& _name,
		const std::vector<par::ASTExpNode*>::iterator& _begin,
		const std::vector<par::ASTExpNode*>::iterator& _end,
		std::vector<NaReTi::Module::FuncMatch>& _funcQuery)
	{
		for (auto& module : m_modules)
		{
			Function* func = module->getFunction(_name, _begin, _end, _funcQuery);
			if (func) return func;
		}
		return nullptr;
	}

	VarSymbol* ModuleLibrary::getGlobalVar(const std::string& _name)
	{
		for (auto& module : m_modules)
		{
			VarSymbol* var = module->getGlobalVar(_name);
			if (var) return var;
		}
		return nullptr;
	}
}