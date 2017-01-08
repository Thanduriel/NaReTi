#include "generics.hpp"
#include "module.hpp"
#include <fstream>
#include <assert.h>

namespace par{
	using namespace std;

	GenericsParser* g_genericsParser;

	std::string GenericsParser::mangledName(const std::string& _name, const std::vector<std::string>& _args)
	{
		string mangled = _name + "<";
		for (auto& argName : _args)
			mangled += argName + ",";
		mangled += ">";

		return std::move(mangled);
	}

	void GenericsParser::parseType(const std::string& _name, int _argCount, ComplexType* _args)
	{
		assert(m_targetModule);

		for (int i = 0; i < _argCount; ++i)
		{
			string name = "__T_" + std::to_string(i);

			m_targetModule->m_typeAlias.emplace_back(name, &_args[i]);
			//give it a name which the semantic parser will find
		}
		//the last function of the target module could be the one where the generic type first occurs
		//to have generic functions compiled before this function is moved to the back after
		auto ptr = std::move(m_targetModule->m_functions.back());
		m_targetModule->m_functions.pop_back();

		string fileContent = m_loader.load(_name);

		//since parsing happens in the middle of a different file
		auto restoreIt = g_lastIterator;

		parse(fileContent, *m_targetModule);

		g_lastIterator = restoreIt;
		m_targetModule->m_functions.push_back(std::move(ptr));
	}

	// ***************************************************************** //

	GenericTrait::GenericTrait(const std::vector<std::string>& _params)
	{
		for (size_t i = 0; i < _params.size(); ++i)
		{
			m_typeParams.emplace_back(new ComplexType(_params[i]));
			m_typeParams.back()->size = i;
		}
	}

	// ***************************************************************** //

	GenericType::GenericType(const std::string& _name, const std::vector<std::string>& _params):
		ComplexType(_name),
		GenericTrait(_params)
	{
	}

	// ***************************************************************** //

	ComplexType* GenericType::getSpecialization(ComplexType* _params)
	{
		//construct name
		string newName = name + "<";
		for (int i = 0; i < (int)m_typeParams.size(); ++i)
			newName += _params[i].name + ",";

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
			auto size = var->typeInfo.type.size;
			//is a generic type
			if (size != ComplexType::UndefSize) type.scope.m_variables.emplace_back(new VarSymbol(var->name, TypeInfo(_params[size], var->typeInfo)));
			else type.scope.m_variables.emplace_back(new VarSymbol(var->name, TypeInfo(var->typeInfo))); //copy completely
		}

		return &type;
	}

	// ***************************************************************** //

	GenericFunction::GenericFunction(const std::string& _name, TypeInfo& _info, const std::vector<std::string>& _params) :
		GenericTrait(_params),
		Function(_name, _info)
	{

	}

	Function* makeSpecialisation(const std::string& _name, ComplexType* _params)
	{
		return nullptr;
	}

	// ***************************************************************** //

	ASTNode* GenericFunction::copyTree(ASTNode* _node, utils::DetorAlloc& _alloc)
	{
		switch (_node->type)
		{
		case ASTType::Code:
		{
			ASTCode& node = *(ASTCode*)_node;
			ASTCode* newNode = _alloc.construct< ASTCode >(node);

			for (int i = 0; i < (int)node.size(); ++i)
			{
				(*newNode)[i] = copyTree(node[i], _alloc);
			}
			return newNode;
		}
	/*	case ASTType::Branch:
		{
			ASTBranch& node = *(ASTBranch*)_node;
			ASTBranch* newNode = _alloc.construct< ASTBranch >(node);
			
			 
			return newNode;
		}
		case ASTType::Loop:
			traceLoop(*(ASTLoop*)_node);
			break;
		case ASTType::Call:
			traceCall(*(ASTCall*)_node, _dest);
			break;
		case ASTType::Ret:
			traceReturn(*(ASTReturn*)_node);
			break;
		case ASTType::Member:
			traceMember(*(ASTMember*)_node);
			break;
		case ASTType::LeafSym:
			traceLeaf(*(ASTLeafSym*)_node);
			break;*/
		}
		return nullptr;
	}
}