#pragma once

#include <boost/fusion/container/vector.hpp>
#include <boost/optional.hpp>

#include "module.hpp"
#include "symbols.hpp"
#include "modulelib.hpp"
#include "ast.hpp"
#include "stackalloc.hpp"

namespace par
{
	/* SemanticParser *******************************
	* Performs all semantic actions evoked by the boost parser.
	* Generates symbol tables and an abstract syntax tree.
	* All source code errors are thrown by the SemanticParser
	* and the finished module should be valid when no error is found.
	*/
	class SemanticParser
	{
	public:
		SemanticParser();

		//set the module that takes the parsed symbols
		void setModule(NaReTi::Module& _module);

		//set the scope back to the default scope of the module
		void resetScope() { m_currentCode = &m_currentModule->m_text; m_currentScope = &m_currentModule->m_text; };

		void varDeclaration(std::string& _attr);
		void pushLatestVar();
		void typeDeclaration(std::string& _attr);
		void useStatement(std::string& _attr);
		void finishTypeDec();
		void funcDeclaration(std::string& _attr);
		void finishParamList(); // finish the param list of the currently parsed function
		//typeinfo
		void newTypeInfo(const std::string& _attr) { m_typeName = _attr; m_typeInfo.isReference = false; m_typeInfo.isConst = false; }
		void makeReference() { m_typeInfo.isReference = true; }
		void makeConst() { m_typeInfo.isConst = true; }
		void makeArray() { m_typeInfo.isArray = true; }
		void setArraySize(int _val) { m_typeInfo.arraySize = _val; }
		void makeExternal() { m_currentFunction->bExternal = true; }
		void finishGeneralExpression();
		void beginCodeScope();
		void finishCodeScope();
		void ifConditional();
		void elseConditional();
		void loop();
		void returnStatement();

		//term parsing
		/* The general building process works like this:
		 * * all symbols are pushed onto the stack.
		 * * expressions(stuff that returns smth) take required operands from the stack and put the assembled node back.
		 * * statements (e.g. return) take required operands from the stack and output the nodes to the current codescope.
		 */
		void pushSymbol(std::string& _name);
		void pushFloat(double _val);
		void pushInt(int _val);
		void pushString(std::string& _str);
		void term(const std::string& _operator);
		void call(std::string& _name);
		void argSeperator();

		void lockLatestNode();
		//looks in the tree with the given node as root for the right position of the top level on the stack.
		ASTExpNode** findPrecPos(ASTExpNode** _tree, ASTCall& _node);
	private:
		ASTExpNode* popNode() { ASTExpNode* ptr = m_stack.back(); m_stack.pop_back(); if (ptr->type == ASTType::Call || ptr->type == ASTType::Member){ linkCall(*(ASTCall*)ptr); } return ptr; };

		void linkCall(ASTCall& _node);
		void linkMember(ASTMember& _node);

		/* tries to add typecasts to the args to make them match the given function.
		 * This will not perform any changes to the tree if it is unsuccessful.
		 * @return success
		 */
		bool tryArgCasts(ASTCall& _node, Function& _func);

		TypeInfo buildTypeInfo();

		std::vector < ASTExpNode* > m_stack;
		TypeInfo m_typeInfo;
		std::string m_typeName;

		std::vector<NaReTi::Module::FuncMatch> m_funcQuery;

		NaReTi::Module* m_currentModule; // the module that is currently parsed to
		par::Function* m_currentFunction; // currently parsed function
		par::ModuleLibrary m_moduleLib;
		CodeScope* m_currentScope;
		ASTCode* m_currentCode;
		ASTCode* m_targetScope; // a scope that is to be entered

		//the current module's allocator
		utils::StackAlloc* m_allocator;
	};
	 // simple print for debugging
	void testFunc();
	void testFuncStr(std::string& _str);

}