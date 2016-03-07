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
		m_currentCode = &_module.m_text; 
		m_allocator = &_module.getAllocator();
		m_moduleLib.reset(); 

		//the current module is a valid resource aswell
		m_moduleLib.addModule(_module);
	}

	// ************************************************** //

	void SemanticParser::linkCall(ASTCall& _node)
	{
		for (auto& arg : _node.args)
		{
			//only not linked functions have no typeinfo at this point
			if (arg->typeInfo == nullptr) linkCall(*(ASTCall*)arg);
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
		if (!type) throw ParsingError("Unknown type");

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
			if (!type) throw ParsingError("Unkown type");
			m_currentModule->m_functions.emplace_back(new Function(_attr.m1.get(), *type));
		}
		//assume void
		else
		{
			m_currentModule->m_functions.emplace_back(new Function(_attr.m0, *m_moduleLib.getType("void")));
		}

		//init envoirement
		m_currentFunction = m_currentModule->m_functions.back().get();
		m_targetScope = &m_currentModule->m_functions.back()->scope;
		m_targetScope->m_parent = m_currentScope;
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
	}

	// ************************************************** //

	void SemanticParser::term(string& _operator)
	{
		//the plan: -pop the top 2 stack params
		// -find the matching function using them
		// -add a call instruction
		// -push m_accumulator with the updated type on the stack
		if (_operator == ".") return; // member access is currently handled in pushSymbol

		//build new node
		ASTCall* astNode = m_allocator->construct<ASTCall>();
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
	//	m_stack.push_back(astNode);
		cout << _operator << endl;
	}

	// ************************************************** //

	void SemanticParser::pushSymbol(string& _name)
	{
		VarSymbol* var = m_currentCode->getVar(_name);
		if (!var)
		{
			size_t i = 0;
			// check members of the type on top of the stack
			for (auto& member : m_stack.back()->typeInfo->type.scope.m_variables)
			{
				if (member.name == _name)
				{
					ASTMember* memberNode = m_allocator->construct<ASTMember>(*popNode(), i);
					memberNode->typeInfo = &member.typeInfo;
					m_stack.push_back(memberNode);

					return;
				}
				i++;
			}

			throw ParsingError("Unknown symbol");

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

	void SemanticParser::popParam()
	{
/*		par::InstructionType instrT;
		switch (m_paramStack.back().type)
		{
		case ParamType::PtrFunc: instrT = InstructionType::Call;
			break;
		case ParamType::Ptr:
		case ParamType::Float:
		case ParamType::Int: instrT = InstructionType::Push;
			break;
		}

		m_currentCode->m_instructions.emplace_back(instrT, m_paramStack.back());
		m_paramStack.pop_back();*/
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