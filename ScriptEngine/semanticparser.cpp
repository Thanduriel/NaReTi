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
		Type* type = m_moduleLib.getType(_attr.m0);
		if (!type) throw ParsingError("Unkown type");

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

	void SemanticParser::funcDeclaration(boost::fusion::vector2< std::string, boost::optional <std::string> >& _attr)
	{
		//a type is provided
		if (_attr.m1.is_initialized())
		{
			Type* type = m_moduleLib.getType(_attr.m0);
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
		m_currentCode = &m_currentModule->m_functions.back()->scope;
		m_currentScope = &m_currentModule->m_functions.back()->scope;
		//destruct previous tree
		m_allocator->reset();
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

	void SemanticParser::returnStatement()
	{
		m_currentCode->emplace_back(m_allocator->construct<ASTReturn>());
		ASTReturn& retNode = *(ASTReturn*)m_currentCode->back();

		if (m_currentFunction->returnType.basic != BasicType::Void)
		{
			retNode.body = popNode();
		}
	}

	// ************************************************** //

	void SemanticParser::term(string& _operator)
	{
		//the plan: -pop the top 2 stack params
		// -find the matching function using them
		// -add a call instruction
		// -push m_accumulator with the updated type on the stack
		if (_operator == ".") return; // member access is currently handled in pushSymbol

		Function* func = m_moduleLib.getFunction(_operator, m_stack.begin() + (m_stack.size() - 2), m_stack.end());
		if (!func) throw ParsingError("No function with the given signature found.");

		//build new node
		ASTCall* astNode = m_allocator->construct<ASTCall>();
		astNode->function = func;
		astNode->expType = (ComplexType*)&func->returnType;

		//pop the used params
		astNode->args.resize(2);
		astNode->args[1] = popNode();
		//traverse the tree to find the right position in regard of precedence
		ASTExpNode** dest = findPrecPos(&m_stack[m_stack.size() - 1], *astNode);
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
			for (auto& member : m_stack.back()->expType->scope.m_variables)
			{
				if (member.name == _name)
				{
					ASTMember* memberNode = m_allocator->construct<ASTMember>(*popNode(), i);
					memberNode->expType = (ComplexType*)&member.type;
					m_stack.push_back(memberNode);

					return;
				}
				i++;
			}

			throw ParsingError("Unkown symbol");

		}
		
		ASTLeaf* leaf = m_allocator->construct<ASTLeaf>(var);
		leaf->expType = (ComplexType*)&var->type;
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
		leaf->expType = &lang::g_module.getBasicType(BasicType::Int);
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
			if (!call.isLocked && lang::g_module.getPrecedence(call.function->name) > lang::g_module.getPrecedence(_node.function->name))
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