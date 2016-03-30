#pragma once
#include "module.hpp"

namespace lang{
	// Generates modules with operations for specific typed arrays.
	class ArrayTypeGen
	{
	public:
		//destination module where the operations are pushed
		void build(par::ComplexType& _type);
	private:
		void buildAppend();
		//void buildCombine(); might not be that useful
		void buildElemAccess();

		par::ComplexType* m_currentType;
		NaReTi::Module m_module;
		std::vector< par::ComplexType* > m_buildTypes; // types already build
	};
}