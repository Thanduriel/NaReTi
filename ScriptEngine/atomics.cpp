#include "atomics.hpp"
#include "symbols.hpp"

namespace lang
{
	using namespace par;

	BasicModule g_module;

	BasicModule::BasicModule():
		Module("")
	{
		//basic types
		m_types.emplace_back("int", BasicType::Int);
		m_types.emplace_back("float", BasicType::Float);
		m_types.emplace_back("string", BasicType::String);
		m_types.emplace_back("void", BasicType::Void);

		//operators
		//basic arithmetic
		m_functions.emplace_back("+", m_types[0], InstructionType::Add);
		m_functions.emplace_back("-", m_types[0], InstructionType::Sub);
		m_functions.emplace_back("*", m_types[0], InstructionType::Mul);
		m_functions.emplace_back("/", m_types[0], InstructionType::Div);
		//assignment
		m_functions.emplace_back("=", m_types[0], InstructionType::Set);
	}
}