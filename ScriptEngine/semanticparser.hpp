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
		void resetScope() { m_currentCode = &m_currentModule->m_text; };

		void makeReference() { m_isReference = true; };
		void varDeclaration(boost::fusion::vector3< std::string, boost::optional<char>, std::string >& _attr);
		void typeDeclaration(std::string& _attr);
		void funcDeclaration(boost::fusion::vector2< std::string, boost::optional <std::string >>& _attr);
		void finishParamList(); // finish the param list of the currently parsed function
		void finishGeneralExpression();
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
		void term(std::string& _operator);

		void lockLatestNode();
		//looks in the tree with the given node as root for the right position of the top level on the stack.
		ASTExpNode** findPrecPos(ASTExpNode** _tree, ASTCall& _node);
	private:
		//pops a element from the param stack and translates it into an instruction
		void popParam();
		ASTExpNode* popNode() { ASTExpNode* ptr = m_stack.back(); m_stack.pop_back(); return ptr; };
		std::vector < ASTExpNode* > m_stack;

		NaReTi::Module* m_currentModule; // the module that is currently parsed to
		par::Function* m_currentFunction; // currently parsed function
		par::ModuleLibrary m_moduleLib;
		CodeScope* m_currentScope;
		ASTCode* m_currentCode;

		//the current module's allocator
		utils::StackAlloc* m_allocator;

		//flags
		bool m_isReference;
	};

	 // simple print for debugging
	void testFunc();
	void testFuncStr(std::string& _str);

}