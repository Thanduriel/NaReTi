#include "semanticparser.hpp"
#include "atomics.hpp"
#include <string>
#include <iostream>
#include "parexception.hpp"

using namespace std;

namespace par
{
	SemanticParser::SemanticParser() :
		m_moduleLib(lang::g_module)
	{

	}

	// ************************************************** //

	void SemanticParser::setModule(NaReTi::Module& _module)
	{ 
		m_currentModule = &_module; 
		resetScope();
		m_allocator = &_module.getAllocator();
		m_moduleLib.reset(); 
		for (auto& dep : _module.m_dependencies)
			m_moduleLib.addModule(*dep);

		//the current module is a valid resource as well
		m_moduleLib.addModule(_module);
	}

	// ************************************************** //

	void SemanticParser::linkCall(ASTCall& _node)
	{
		for (auto& arg : _node.args)
		{
			//only not linked functions have no typeinfo at this point
			if (arg->typeInfo == nullptr && (arg->type == ASTType::Call || arg->type == ASTType::Member))
			{
				ASTCall& call = *(ASTCall*)arg;
				linkCall(call);
			}
		}

		if (_node.type == ASTType::Member)
		{
			ASTMember& memberNode = *(ASTMember*)&_node;
			ASTUnlinkedSym* strLeaf = (ASTUnlinkedSym*)_node.args[1];
			size_t i = 0;
			// check members of the type on top of the stack
			for (auto& member : _node.args[0]->typeInfo->type.scope.m_variables)
			{
				if (member.name == strLeaf->name)
				{
					memberNode.typeInfo = &member.typeInfo;
					memberNode.index = 0;
					memberNode.instance = memberNode.args[0];
					return;
				}
				i++;
			}

			throw ParsingError(_node.args[0]->typeInfo->type.name + " has no member called \"" + strLeaf->name + "\"");
		}

		Function* func = m_moduleLib.getFunction(_node.name, _node.args.begin(), _node.args.end());
		if (!func) throw ParsingError("No function with the given signature found.");

		_node.function = func;
		_node.typeInfo = &func->returnTypeInfo;
	}

	// ************************************************** //
	// boost parser declarations						  //
	// ************************************************** //

	using namespace boost::fusion;

	std::string fa(boost::fusion::vector2<char, std::vector<char>>& _attr)
	{
		std::string str;
		str.resize(_attr.m1.size() + 1);
		str[0] = _attr.m0;
		for (int i = 0; i < _attr.m1.size(); ++i)
			str[i + 1] = _attr.m1[i];

		//	std::cout << str << std::endl;

		return str;
	}
	void SemanticParser::varDeclaration(vector3< string, boost::optional<char>, string >& _attr)
	{
		//search type
		ComplexType* type = m_moduleLib.getType(_attr.m0);
		if (!type) throw ParsingError("Unknown type: " + _attr.m0);

		m_currentScope->m_variables.emplace_back(_attr.m2, *type, _attr.m1.is_initialized());
		//	std::cout << "var declaration" << _attr.m0 << " " << _attr.m1 << endl;
	}

	// ************************************************** //

	void SemanticParser::typeDeclaration(std::string& _attr)
	{
		m_currentModule->m_types.emplace_back(new ComplexType(_attr));
		m_currentScope = &m_currentModule->m_types.back()->scope;
	}

	// ************************************************** //

	void SemanticParser::finishTypeDec()
	{
		// generate default assigment

	}

	// ************************************************** //

	void SemanticParser::funcDeclaration(boost::fusion::vector2< std::string, boost::optional <std::string> >& _attr)
	{
		//a type is provided
		if (_attr.m1.is_initialized())
		{
			ComplexType* type = m_moduleLib.getType(_attr.m0);
			if (!type) throw ParsingError("Unkown type: " + _attr.m0);
			m_currentModule->m_functions.emplace_back(new Function(_attr.m1.get(), *type));
		}
		//assume void
		else
		{
			m_currentModule->m_functions.emplace_back(new Function(_attr.m0, *m_moduleLib.getType("void")));
		}

		//init envoirement
		m_currentFunction = m_currentModule->m_functions.back().get();
		m_currentFunction->bInline = false;
		m_targetScope = &m_currentModule->m_functions.back()->scope;
		m_targetScope->m_parent = m_currentScope;
		//param dec is outside of the following code scope
		m_currentScope = m_targetScope;
	}

	void SemanticParser::finishParamList()
	{
		Function& function = *m_currentModule->m_functions.back();
		function.paramCount = (int)function.scope.m_variables.size();
	}

	// ************************************************** //

	void SemanticParser::finishGeneralExpression()
	{
		while (m_stack.size())
			m_currentCode->push_back(popNode());
	}

	// ************************************************** //

	void SemanticParser::beginCodeScope()
	{
		ASTCode* codeNode;
		if (m_targetScope)
		{
			codeNode = m_targetScope;
		}
		else
		{
			codeNode = m_allocator->construct<ASTCode>();
			//since the node is linked to no other node it is placed into the currently open scope
			m_currentCode->push_back(codeNode);
		}
		m_targetScope = nullptr;
		codeNode->parent = m_currentCode;
		codeNode->m_parent = m_currentCode;
		m_currentCode = codeNode;
		m_currentScope = codeNode;
	}

	// ************************************************** //

	void SemanticParser::finishCodeScope()
	{
		//return to the previous scope
		m_currentCode = m_currentCode->parent;
		m_currentScope = m_currentScope->m_parent;
	}

	// ************************************************** //

	void SemanticParser::ifConditional()
	{
		ASTBranch& branchNode = *m_allocator->construct<ASTBranch>();
		branchNode.condition = popNode();
		branchNode.ifBody = m_allocator->construct<ASTCode>();
		m_targetScope = branchNode.ifBody;

		m_currentCode->push_back(&branchNode);
	}

	// ************************************************** //

	void SemanticParser::elseConditional()
	{
		ASTBranch& parentNode = *(ASTBranch*)m_currentCode->back();
		parentNode.elseBody = m_allocator->construct<ASTCode>();
		m_targetScope = parentNode.elseBody;
		beginCodeScope();
	}

	// ************************************************** //

	void SemanticParser::returnStatement()
	{
		m_currentCode->emplace_back(m_allocator->construct<ASTReturn>());
		ASTReturn& retNode = *(ASTReturn*)m_currentCode->back();

		if (m_currentFunction->returnTypeInfo.type.basic != BasicType::Void)
		{
			retNode.body = popNode();
		}
	}

	// ************************************************** //

	void SemanticParser::loop()
	{
		ASTLoop& node = *m_allocator->construct<ASTLoop>();
		node.condition = popNode();
		node.body = m_allocator->construct<ASTCode>();
		m_targetScope = node.body;

		m_currentCode->push_back(&node);
	}

	// ************************************************** //

	void SemanticParser::term(string& _operator)
	{
		//build new node
		ASTCall* astNode = _operator == "." ? m_allocator->construct<ASTMember>() : m_allocator->construct<ASTCall>();
		astNode->name = _operator;
		astNode->typeInfo = nullptr;

		//pop the used params
		astNode->args.resize(2);
		//no popNode since the tree is not final
		astNode->args[1] = m_stack.back(); m_stack.pop_back();
		//traverse the tree to find the right position in regard of precedence
		ASTExpNode** dest = findPrecPos(&m_stack.back(), *astNode);
		astNode->args[0] = *dest;
		*dest = astNode; // put this node there

		cout << _operator << endl;
	}

	void SemanticParser::call(string& _name)
	{
		//build new node
		ASTCall* node = m_allocator->construct<ASTCall>();
		node->name = _name;
		node->typeInfo = nullptr;

		m_stack.push_back(node);
	}

	void SemanticParser::argSeperator()
	{
		ASTExpNode* expNode = m_stack.back(); m_stack.pop_back();
		ASTCall* callNode = (ASTCall*)m_stack.back();
		callNode->args.push_back(expNode);
	}

	// ************************************************** //

	void SemanticParser::pushSymbol(string& _name)
	{
		VarSymbol* var;
		//look in global scope
		var = m_moduleLib.getGlobalVar(_name);
		if (var)
		{
			bool isImported = false;
			for (auto& impVar : m_currentFunction->m_importedVars) if (impVar == var){ isImported = true; break; }
			if(!isImported) m_currentFunction->m_importedVars.push_back(var);
		}
		else var = m_currentCode->getVar(_name);

		if (!var) //could still be a member
		{
			m_stack.push_back(m_allocator->construct<ASTUnlinkedSym>(_name));
			return;
		}
		
		ASTLeaf* leaf = m_allocator->construct<ASTLeaf>(var);
		leaf->typeInfo = &var->typeInfo;
		m_stack.push_back(leaf);
		cout << _name << endl;
	}

	void SemanticParser::pushFloat(double _val)
	{
//		m_paramStack.emplace_back((float)_val);
		cout << _val << endl;
	}

	void SemanticParser::pushInt(int _val)
	{
		ASTLeaf* leaf = m_allocator->construct<ASTLeaf>(_val);
		leaf->typeInfo = m_allocator->construct<TypeInfo>(lang::g_module.getBasicType(BasicType::Int), false);
		m_stack.push_back(leaf);
		cout << _val << endl;
	}

	// ************************************************** //

	void SemanticParser::lockLatestNode()
	{
		((ASTCall*)m_stack.back())->isLocked = true;
	}

	// ************************************************** //

	ASTExpNode** SemanticParser::findPrecPos(ASTExpNode** _tree, ASTCall& _node)
	{
		switch ((*_tree)->type)
		{
		case ASTType::Call:
		{
			ASTCall& call = *((ASTCall*)(*_tree));
			if (!call.isLocked && lang::g_module.getPrecedence(call.name) > lang::g_module.getPrecedence(_node.name))
			{
				return (ASTExpNode**)&call.args[1];
			}
			break;
		}
		case ASTType::Leaf:
		case ASTType::Member:
			return _tree;
		}
		return _tree;
	}

	// ************************************************** //

	void testFunc()
	{
		std::cout << "test got called!" <<  std::endl;
	}

	void testFuncStr(string& str)
	{
		std::cout << str << std::endl;
	}
}