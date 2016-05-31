#include "generics.hpp"
#include "module.hpp"

namespace par{
	using namespace std;


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