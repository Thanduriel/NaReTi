#pragma once

#include "symbols.hpp"
#include "modulelib.hpp"

namespace lang{
	/* 
	 * generates functions for any types
	 */

	class TypeDefaultGen
	{
	public:
		void buildElemAccess(par::ComplexType& _type, NaReTi::Module& _module);

		void buildDefaultAssignment(par::ComplexType& _type, NaReTi::Module& _module, par::ModuleLibrary& _lib);
	private:
		std::vector<NaReTi::Module::FuncMatch> m_funcQuery;
	};
}