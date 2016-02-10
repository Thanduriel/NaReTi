#pragma once

#include "symbols.hpp"
#include "ast.hpp"
#include <vector>
#include <string>

namespace NaReTi{

	class Module
	{
	public:
		Module(const std::string& _name) : m_name(_name){};

		par::Type* getType(const std::string& _name);
		/*search for a function with the given signature
		 * _name the name of the function 
		 * _ret the return type
		 * _begin, _end range on a param stack with the given arguments
		 */
		par::Function* getFunction(const std::string& _name, 
			const std::vector<par::ASTNode*>::iterator& _begin,
			const std::vector<par::ASTNode*>::iterator& _end);

		std::vector < par::ComplexType > m_types;
		std::vector < par::Function > m_functions;
		par::ASTCode m_text;

	private:
		std::string m_name;
	};
}