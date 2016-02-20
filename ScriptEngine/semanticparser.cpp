#include "semanticparser.hpp"
#include "atomics.hpp"
#include <string>
#include <iostream>
#include "parexception.hpp"

using namespace std;

namespace par
{
	SemanticParser::SemanticParser() :
		m_moduleLib(lang::g_module),
		m_accumulator("", *lang::g_module.getType("int")),
		m_accParam(&m_accumulator)
	{

	}

	// ************************************************** //

	void SemanticParser::setModule(NaReTi::Module& _module)
	{ 
		m_currentModule = &_module; 
		m_currentCode = &_module.m_text; 
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
	void SemanticParser::varDeclaration(vector2< string, string >& _attr)
	{
		//search type
		Type* type = m_moduleLib.getType(_attr.m0);
		if (!type) throw ParsingError("Unkown type");

		m_currentScope->m_variables.emplace_back(_attr.m1, *type);
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
		m_allocator.reset();
	}

	void SemanticParser::finishParamList()
	{
		Function& function = *m_currentModule->m_functions.back();
		function.paramCount = (int)function.scope.m_variables.size();
	}

	// ************************************************** //

	void SemanticParser::returnStatement()
	{
		m_currentCode->emplace_back(m_allocator.construct<ASTReturn>());
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
		Function* func = m_moduleLib.getFunction(_operator, m_stack.begin() + (m_stack.size() - 2), m_stack.end());
		if (!func) throw ParsingError("No function with the given signiture found.");

		ASTNode* node;

		ASTCall* astNode = m_allocator.construct<ASTCall>();
		astNode->function = func;
		//pop the used params
		astNode->args.resize(2);
		astNode->args[1] = popNode();
		astNode->args[0] = popNode();
		node = astNode;

		//add result
		m_stack.push_back(node);
	//	cout << _operator << endl;
	}

	// ************************************************** //

	void SemanticParser::pushSymbol(string& _name)
	{
		VarSymbol* var = m_currentCode->getVar(_name);
		if (!var) throw ParsingError("Unkown symbol");

		m_stack.push_back(m_allocator.construct<ASTLeaf>(var));
		cout << _name << endl;
	}

	void SemanticParser::pushFloat(double _val)
	{
//		m_paramStack.emplace_back((float)_val);
		cout << _val << endl;
	}

	void SemanticParser::pushInt(int _val)
	{
		m_stack.push_back(m_allocator.construct<ASTLeaf>(_val));
		cout << _val << endl;
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