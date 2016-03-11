#pragma once

#include "symbols.hpp"
#include "ast.hpp"
#include "stackalloc.hpp"
#include <vector>
#include <string>
#include <memory>

namespace NaReTi{

	class Module
	{
	public:
		Module(const std::string& _name) : m_name(_name){};

		par::ComplexType* getType(const std::string& _name);
		/*search for a function with the given signature
		 * _name the name of the function 
		 * _ret the return type
		 * _begin, _end range on a param stack with the given arguments
		 */
		par::Function* getFunction(const std::string& _name, 
			const std::vector<par::ASTExpNode*>::iterator& _begin,
			const std::vector<par::ASTExpNode*>::iterator& _end);

		par::Function* getFunction(const std::string& _name);

		par::VarSymbol* getGlobalVar(const std::string& _name);

		utils::StackAlloc& getAllocator() { return m_allocator; }

		//external construction
		bool linkExternal(const std::string& _name, void* _funcPtr);

		//their lifetime equals the module's
		std::vector < std::unique_ptr<par::ComplexType> > m_types;
		std::vector < std::unique_ptr<par::Function> > m_functions;
		par::ASTCode m_text;

		std::string m_name;

		std::vector < Module* > m_dependencies;
	protected:

		// allocator for the ast-nodes
		// todo: types and functions could be allocated by this as well to make destruction faster
		utils::StackAlloc m_allocator;
	};
}