#include "atomics.hpp"
#include "symbols.hpp"

namespace lang
{
	using namespace par;

	BasicModule g_module;

	BasicModule::BasicModule():
		Module("")
	{
		m_types.resize(5);
		//basic types
		m_types[BasicType::Int] = par::ComplexType("int", BasicType::Int);
		m_types[BasicType::Float] = par::ComplexType("float", BasicType::Float);
		m_types[BasicType::String] = par::ComplexType("string", BasicType::String);
		m_types[BasicType::Void] = par::ComplexType("void", BasicType::Void);
		m_types[BasicType::Bool] = par::ComplexType("bool", BasicType::Bool);

		//operators
		//basic arithmetic
		m_functions.emplace_back("+", m_types[0], InstructionType::Add);
		m_functions.emplace_back("-", m_types[0], InstructionType::Sub);
		m_functions.emplace_back("*", m_types[0], InstructionType::Mul);
		m_functions.emplace_back("/", m_types[0], InstructionType::Div);
		//assignment
		m_functions.emplace_back("=", m_types[0], InstructionType::Set);
	}

	par::ComplexType& BasicModule::getBasicType(par::BasicType _basicType)
	{
		return m_types[_basicType];
	}
}