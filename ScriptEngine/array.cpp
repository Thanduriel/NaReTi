#include "array.hpp"
#include "atomics.hpp"

namespace lang{
	using namespace par;

	void ArrayTypeGen::build(par::ComplexType& _type)
	{
		//already exists
		for (auto& type : m_buildTypes)
			if (&_type == type) return;

		m_currentType = &_type;
	}

	void ArrayTypeGen::buildAppend()
	{
		m_module.m_functions.emplace_back(new Function("+=", TypeInfo(g_module.getBasicType(BasicType::Void))));
	}

	void ArrayTypeGen::buildElemAccess()
	{
		m_module.m_functions.emplace_back(new Function("[]", TypeInfo(*m_currentType, true)));
	}
}