#include "generics.hpp"
#include "module.hpp"

namespace par{
	using namespace std;

	GenericType::GenericType(const std::string& _name, const std::vector<std::string>& _params):
		ComplexType(_name)
	{
		for (auto& parName : _params)
		{
			m_typeParams.emplace_back(new ComplexType(parName));
		}
	}

	// ***************************************************************** //

	ComplexType* GenericType::getSpecialization(ComplexType* _params)
	{
		//construct name
		string newName = name + "<";
		for (int i = 0; i < (int)m_typeParams.size(); ++i)
			newName += _params[i].name + ",";

/*		auto it = m_specializations.find(newName);

		if (it == m_specializations.end()) m_specializations.emplace(newName, makeSpecialisation(newName, _params));

		return m_specializations[newName].get();*/
		return nullptr;
	}

	std::string GenericType::mangledName(ComplexType* _params)
	{
		string mangled = name + "<";
		for (int i = 0; i < (int)m_typeParams.size(); ++i)
			mangled += _params[i].name + ",";
		name += ">";

		return name;
	}

	ComplexType* GenericType::makeSpecialisation(const std::string& _name, ComplexType* _params)
	{
		ComplexType& type = *new ComplexType(_name);

		for (auto& var : scope.m_variables)
		{
			int id = -1;
			//look whether the var uses a generic type
			for (int i = 0; i < (int)m_typeParams.size(); ++i) 
				if (m_typeParams[i]->name == var->typeInfo.type.name) { id = i; break; }
			
			if (id != -1) type.scope.m_variables.emplace_back(new VarSymbol(var->name, TypeInfo(_params[id], var->typeInfo)));
			else type.scope.m_variables.emplace_back(new VarSymbol(var->name, TypeInfo(var->typeInfo))); //copy completely
		}

		return &type;
	}
}