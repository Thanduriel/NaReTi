#pragma once
#include "module.hpp"

namespace lang{
	// Generates modules with operations for specific typed arrays.
	class ArrayTypeGen
	{
	public:
		//build array functions for the given type
		void build(par::ComplexType& _type);
		//build array functions for a const array
		void buildConst(par::ComplexType& _type);
	private:
		//creates a new module with the given name if it is not found in m_modules
		//Returns nullptr if it was already created.
		NaReTi::Module* createModule(std::string& _name);
		void buildAppend();
		//void buildCombine(); might not be that useful
		void buildElemAccess();

		par::ComplexType* m_currentType;
		std::vector< std::unique_ptr< NaReTi::Module >> m_modules;
		NaReTi::Module* m_currentModule;
		std::vector< par::ComplexType* > m_buildTypes; // types already build
		std::vector< par::ComplexType* > m_buildConstTypes;
	};
}