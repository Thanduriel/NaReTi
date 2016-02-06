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
		m_currentScope = &_module.m_text; 
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
		m_currentModule->m_types.emplace_back(_attr);
		m_currentScope = &m_currentModule->m_types.back().scope;
	}

	// ************************************************** //

	void SemanticParser::funcDeclaration(boost::fusion::vector2< std::string, boost::optional <std::string> >& _attr)
	{
		//a type is provided
		if (_attr.m1.is_initialized())
		{
			Type* type = m_moduleLib.getType(_attr.m0);
			if (!type) throw ParsingError("Unkown type");
			m_currentModule->m_functions.emplace_back(_attr.m1.get(), *type);
		}
		//assume void
		else
		{
			m_currentModule->m_functions.emplace_back(_attr.m0, *m_moduleLib.getType("void"));
		}
		m_currentScope = &m_currentModule->m_functions.back().scope;
		m_currentFunction = &m_currentModule->m_functions.back();
	}

	void SemanticParser::finishParamList()
	{
		Function& function = m_currentModule->m_functions.back();
		function.paramCount = (int)function.scope.m_variables.size();
	}

	// ************************************************** //

	void SemanticParser::returnStatement()
	{
		if (m_currentFunction->returnType.basic == BasicType::Void)
			m_currentScope->m_instructions.emplace_back(InstructionType::Ret, Parameter());
		else if (m_currentFunction->returnType.basic == BasicType::Int)
		{
			if (m_paramStack.back().type == ParamType::PtrFunc)
			{
				popParam();
				m_currentScope->m_instructions.emplace_back(InstructionType::RetA, Parameter());
			}
			else m_currentScope->m_instructions.emplace_back(InstructionType::RetA, m_paramStack.back());
		}
	}

	// ************************************************** //

	void SemanticParser::term(string& _operator)
	{
		//the plan: -pop the top 2 stack params
		// -find the matching function using them
		// -add a call instruction
		// -push m_accumulator with the updated type on the stack
		Function* func = m_moduleLib.getFunction(_operator, m_paramStack.begin() + (m_paramStack.size() - 2), m_paramStack.end());
		if (!func) throw ParsingError("No function with the given signiture found.");

		//pop the used params
		popParam();
		popParam();
		//add result
		m_paramStack.push_back(func);
	//	cout << _operator << endl;
	}

	// ************************************************** //

	void SemanticParser::pushSymbol(string& _name)
	{
		VarSymbol* var = m_currentScope->getVar(_name);
		if (!var) throw ParsingError("Unkown symbol");

		m_paramStack.emplace_back(m_currentScope->getVar(_name));
		cout << _name << endl;
	}

	void SemanticParser::pushFloat(double _val)
	{
		m_paramStack.emplace_back((float)_val);
		cout << _val << endl;
	}

	void SemanticParser::pushInt(int _val)
	{
		m_paramStack.emplace_back(_val);
		cout << _val << endl;
	}

	// ************************************************** //

	void SemanticParser::popParam()
	{
		par::InstructionType instrT;
		switch (m_paramStack.back().type)
		{
		case ParamType::PtrFunc: instrT = InstructionType::Call;
			break;
		case ParamType::Ptr:
		case ParamType::Float:
		case ParamType::Int: instrT = InstructionType::Push;
			break;
		}

		m_currentScope->m_instructions.emplace_back(instrT, m_paramStack.back());
		m_paramStack.pop_back();
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