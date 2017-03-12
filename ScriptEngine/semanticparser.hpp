#pragma once

#include <boost/fusion/container/vector.hpp>
#include <boost/optional.hpp>

#include "module.hpp"
#include "symbols.hpp"
#include "modulelib.hpp"
#include "ast.hpp"
#include "stackalloc.hpp"
#include "array.hpp"
#include "typedefault.hpp"

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
		void resetScope() { m_currentCode = m_currentModule->m_text; m_currentScope = m_currentModule->m_text; };

		void varDeclaration(const std::string& _attr);
		void pushLatestVar();
		void typeDeclaration(const std::string& _attr);
		void genericTypePar(const std::string& _attr);
		void useStatement(const std::string& _attr);
		void constructorDec();
		void destructorDec();
		void finishTypeDec();
		void funcDeclaration(const std::string& _attr);
		void finishParamList(); // finish the param list of the currently parsed function
		// for var in place initializations
		// takes care of const
		void finishInit();
		//typeinfo
		void newTypeInfo(const std::string& _attr) { m_typeName = _attr; m_typeInfo.isReference = false; m_typeInfo.isConst = false; m_typeInfo.isArray = false; }
		void makeReference() { m_typeInfo.isReference = true; }
		void makeConst() { m_typeInfo.isConst = true; }
		void makeArray();
		void setArraySize(int _val) { m_typeInfo.arraySize = _val; }
		void makeExternal() { m_currentFunction->bExternal = true; }
		void makeExport() { m_currentScope->m_variables.back()->isExport = true; }
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
		void pushSymbol(const std::string& _name);
		void pushFloat(double _val);
		void pushInt(int _val);
		void pushAddress(uint64_t _adr);
		void pushString(const std::string& _str);
		void term(const std::string& _operator);
		void unaryTerm(const boost::optional<std::string>& _str);
		void call(const std::string& _name);
		void argSeperator(); // ","
		void sizeOf();

		void lockLatestNode();
		//looks in the tree with the given node as root for the right position of the top level on the stack.
		ASTExpNode** findPrecPos(ASTExpNode** _tree, ASTCall& _node);
	private:
		//take a expression node from the stack and link the containing functions
		ASTExpNode* popNode() { ASTExpNode* ptr = m_stack.back(); m_stack.pop_back(); if (ptr->type == ASTType::Call || ptr->type == ASTType::Member){ linkCall(*(ASTCall*)ptr); } return ptr; };
		//pop a conditional node and verify that it is boolean or can be implicitly casted
		ASTExpNode* popCondNode();

		void linkCall(ASTCall& _node);
		void linkMember(ASTMember& _node);

		/* tries to add typecasts to the args to make them match the given function.
		 * This will not perform any changes to the tree if it is unsuccessful.
		 * @return success
		 */
		bool tryArgCasts(ASTCall& _node, Function& _func);
		//Try a cast between the given _node's type and _type.
		// Returns a nullptr if none is found.
		Function* typeCast(const TypeInfo& _t0, const TypeInfo& _t1) const;
		std::string buildTypeInfoString(const TypeInfo& _t) const;

		/* Constructs a typeinfo from the previously parsed declaration
		 * and resets the parser state.
		 */
		TypeInfo buildTypeInfo();
		//looks for the given typename in the current modulelib
		//throws an error when a type is not found
		ComplexType& getType(const std::string& _name);

		lang::ArrayTypeGen m_arrayTypeGen;
		lang::TypeDefaultGen m_typeDefaultGen;

		std::vector < ASTExpNode* > m_stack;
		TypeInfo m_typeInfo;
		std::string m_typeName;
		std::vector < std::string> m_genericTypeParams;

		std::vector<NaReTi::Module::FuncMatch> m_funcQuery;

		NaReTi::Module* m_currentModule; // the module that is currently parsed to
		par::Function* m_currentFunction; // currently parsed function
		par::ModuleLibrary m_moduleLib;
		CodeScope* m_currentScope;
		ASTCode* m_currentCode;
		ASTCode* m_targetScope; // a scope that is to be entered

		//the current module's allocator
		utils::DetorAlloc* m_allocator;
	};
	 // simple print for debugging
	void testFunc();
	void testFuncStr(std::string& _str);

}