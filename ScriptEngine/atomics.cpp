#include "atomics.hpp"
#include "symbols.hpp"

#define BASICOPERATION(X, Y, Z) m_functions.emplace_back(new Function( X, *m_types[ Y ], Z));

namespace lang
{
	using namespace par;

	BasicModule g_module;

	BasicModule::BasicModule():
		Module("")
	{
		m_types.resize(5);
		//basic types
		m_types[BasicType::Int] = std::unique_ptr<ComplexType>(new ComplexType("int", BasicType::Int));
		m_types[BasicType::Float] = std::unique_ptr<ComplexType>(new ComplexType("float", BasicType::Float));
		m_types[BasicType::String] = std::unique_ptr<ComplexType>(new ComplexType("string", BasicType::String));
		m_types[BasicType::Void] = std::unique_ptr<ComplexType>(new ComplexType("void", BasicType::Void));
		m_types[BasicType::Bool] = std::unique_ptr<ComplexType>(new ComplexType("bool", BasicType::Bool));

		//operators
		//int ---------------------------------------------------------
		//basic arithmetic
		BASICOPERATION("+", BasicType::Int, InstructionType::Add);
		BASICOPERATION("-", BasicType::Int, InstructionType::Sub);
		BASICOPERATION("*", BasicType::Int, InstructionType::Mul);
		BASICOPERATION("/", BasicType::Int, InstructionType::Div);

		//float -------------------------------------------------------
		BASICOPERATION("+", BasicType::Float, InstructionType::fAdd);
		BASICOPERATION("-", BasicType::Float, InstructionType::fSub);
		BASICOPERATION("*", BasicType::Float, InstructionType::fMul);
		BASICOPERATION("/", BasicType::Float, InstructionType::fDiv);
	/*	m_functions.emplace_back("+", m_types[0], InstructionType::Add);
		m_functions.emplace_back("-", m_types[0], InstructionType::Sub);
		m_functions.emplace_back("*", m_types[0], InstructionType::Mul);
		m_functions.emplace_back("/", m_types[0], InstructionType::Div);
		//assignment
		m_functions.emplace_back("=", m_types[0], InstructionType::Set);

		//float --------------------------------------------------------
		m_functions.emplace_back("+", m_types[1], InstructionType::fAdd);
		m_functions.emplace_back("-", m_types[1], InstructionType::fSub);
		m_functions.emplace_back("*", m_types[1], InstructionType::fMul);
		m_functions.emplace_back("/", m_types[1], InstructionType::fDiv);*/
	}

	par::ComplexType& BasicModule::getBasicType(par::BasicType _basicType)
	{
		return *m_types[_basicType];
	}
}