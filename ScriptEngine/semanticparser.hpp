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
	* Does all semantic actions.
	* Translates matches of the boostparser to Symbol definitions.
	*/
	class SemanticParser
	{
	public:
		SemanticParser();

		/* parse() *******************************
		* Translates a tokenizedText into a symbolizedText
		* and adds the result to the given module
		* overriting any symbols that are already defined.
		*/
		//void parse(tok::TokenizedText&& _tokenizedText, NaReTi::Module& _module);

		//set the module that takes the parsed symbols
		void setModule(NaReTi::Module& _module);

		//set the scope back to the default scope of the module
		void resetScope() { m_currentCode = &m_currentModule->m_text; };

		void makeReference() { m_isReference = true; };
		void varDeclaration(boost::fusion::vector3< std::string, boost::optional<char>, std::string >& _attr);
		void typeDeclaration(std::string& _attr);
		void funcDeclaration(boost::fusion::vector2< std::string, boost::optional <std::string >>& _attr);
		void finishParamList(); // finish the param list of the currently parsed function
		void returnStatement();

		//term parsing
		void pushSymbol(std::string& _name);
		void pushFloat(double _val);
		void pushInt(int _val);
		void term(std::string& _operator);
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
		VarSymbol m_accumulator; // a local var that does not have a name
		Parameter m_accParam; // parameter for the accumulator

		utils::StackAllocator<1024> m_allocator; // allocator for the ast

		//flags
		bool m_isReference;
	};

	 // simple print for debugging
	void testFunc();
	void testFuncStr(std::string& _str);

}