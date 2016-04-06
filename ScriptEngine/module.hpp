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
		
		/* Structure that holds function querys.
		 */
		struct FuncMatch
		{
			FuncMatch(par::Function& _func) : function(&_func), diff(0){}
			par::Function* function;
			int diff; // count of params and args that match

			// allow sorting
			bool operator<(FuncMatch& _oth)
			{
				return diff < _oth.diff;
			}
		};
		/*search for a function with the given signature
		 * _name the name of the function 
		 * _begin, _end range on a param stack with the given arguments
		 * _funcQuery Destination where partly matches(same name, param count) are stored
		 * @ret A function with perfect match or nullptr
		 */
		par::Function* getFunction(const std::string& _name, 
			const std::vector<par::ASTExpNode*>::iterator& _begin,
			const std::vector<par::ASTExpNode*>::iterator& _end,
			std::vector<FuncMatch>& _funcQuery);

		// simpler function search that will return the first name match
		par::Function* getFunction(const std::string& _name);

		// get a local var from this module
		par::VarSymbol* getGlobalVar(const std::string& _name);

		utils::StackAlloc& getAllocator() { return m_allocator; }

		//links an as external declared func symbol in this module to the given ptr.
		bool linkExternal(const std::string& _name, void* _funcPtr);

		//their lifetime equals the module's
		//use a stack allocator
		std::vector < std::unique_ptr<par::ComplexType> > m_types;
		std::vector < std::unique_ptr<par::Function> > m_functions;
		par::ASTCode m_text;

		std::string m_name;

		std::vector < Module* > m_dependencies;
	protected:

		// allocator for the ast-nodes
		utils::StackAlloc m_allocator;
	};
}