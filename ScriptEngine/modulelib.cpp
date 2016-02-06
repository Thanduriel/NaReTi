#include "modulelib.hpp"

//#define ALLSUBMODULES (name) for (auto& module : m_modules){ Type* type = module->getType(_name); if (type) return type; } return nullptr;


namespace par
{
	Type* ModuleLibrary::getType(const std::string& _name)
	{
		for (auto& module : m_modules)
		{
			Type* type = module->getType(_name);
			if (type) return type;
		}
		return nullptr;
	}

	par::Function* ModuleLibrary::getFunction(const std::string& _name,
		const std::vector<par::Parameter>::iterator& _begin,
		const std::vector<par::Parameter>::iterator& _end)
	{
		for (auto& module : m_modules)
		{
			Function* func = module->getFunction(_name, _begin, _end);
			if (func) return func;
		}
		return nullptr;
	}
}