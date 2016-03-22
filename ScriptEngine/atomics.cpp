#include "atomics.hpp"
#include "symbols.hpp"

#define BASICASSIGN(Name, T, Instr) m_functions.emplace_back(new Function( m_allocator, Name, *m_types[ T ], Instr)); m_functions.back()->scope.m_variables[0]->typeInfo.isConst = false;
#define BASICOPERATION(X, Y, Z) m_functions.emplace_back(new Function( m_allocator, X, *m_types[ Y ], Z));
#define BASICOPERATIONEXT(Name, InstrList, T0, T1, T2) m_functions.emplace_back(new Function( m_allocator, Name, InstrList, *m_types[ T0 ], m_types[ T1 ].get(), m_types[ T2 ].get()));

namespace lang
{
	using namespace par;
	using namespace std;

	BasicModule g_module;

	BasicModule::BasicModule():
		Module(""),
		m_precedence( { {
			pair<string, int>("++", 2),
			pair<string, int>("--", 2),
			pair<string, int>(".", 2),
			pair<string, int>("*", 5),
			pair<string, int>("%", 5),
			pair<string, int>("/", 5),
			pair<string, int>("+", 6),
			pair<string, int>("-", 6),
			pair<string, int>("<<", 7),
			pair<string, int>(">>", 7),
			pair<string, int>("<", 8),
			pair<string, int>("==", 9),
			pair<string, int>("!=", 9),
			pair<string, int>("&", 10),
			pair<string, int>("^", 11),
			pair<string, int>("|", 12),
			pair<string, int>("&&", 13),
			pair<string, int>("||", 14),
			pair<string, int>("=", 15)
			} }
		)
	{
		m_types.resize(6);
		//basic types
		m_types[BasicType::Int] = std::unique_ptr<ComplexType>(new ComplexType("int", BasicType::Int)); m_types[BasicType::Int]->size = 4;
		m_types[BasicType::Float] = std::unique_ptr<ComplexType>(new ComplexType("float", BasicType::Float)); m_types[BasicType::Float]->size = 4;
		m_types[BasicType::String] = std::unique_ptr<ComplexType>(new ComplexType("string", BasicType::String));
		m_types[BasicType::Void] = std::unique_ptr<ComplexType>(new ComplexType("void", BasicType::Void));
		m_types[BasicType::Bool] = std::unique_ptr<ComplexType>(new ComplexType("bool", BasicType::Bool)); m_types[BasicType::Bool]->size = 4;
		m_types[BasicType::FlagBool] = std::unique_ptr<ComplexType>(new ComplexType("flagBool", BasicType::FlagBool));

		//operators
		//logical -----------------------------------------------------
		BASICOPERATION("&&", BasicType::FlagBool, InstructionType::Nop);
		BASICOPERATION("||", BasicType::FlagBool, InstructionType::Nop);

		//int ---------------------------------------------------------
		//basic arithmetic
		BASICOPERATION("+", BasicType::Int, InstructionType::Add);
		BASICOPERATION("-", BasicType::Int, InstructionType::Sub);
		BASICOPERATION("*", BasicType::Int, InstructionType::Mul);
		BASICOPERATION("/", BasicType::Int, InstructionType::Div);
		BASICOPERATION("%", BasicType::Int, InstructionType::Mod);
		BASICASSIGN("=", BasicType::Int, InstructionType::Set);
		BASICOPERATION("<<", BasicType::Int, InstructionType::ShL);
		BASICOPERATION(">>", BasicType::Int, InstructionType::ShR);
		BASICOPERATION("&", BasicType::Int, InstructionType::And);
		BASICOPERATION("^", BasicType::Int, InstructionType::Xor);
		BASICOPERATION("|", BasicType::Int, InstructionType::Or);
		//comparison
		BASICOPERATIONEXT("==", (std::initializer_list<InstructionType>{ Cmp, JNE }), BasicType::FlagBool, BasicType::Int, BasicType::Int);
		BASICOPERATIONEXT("<", (std::initializer_list<InstructionType>{ Cmp, JNL }), BasicType::FlagBool, BasicType::Int, BasicType::Int);

		//float -------------------------------------------------------
		BASICOPERATION("+", BasicType::Float, InstructionType::fAdd);
		BASICOPERATION("-", BasicType::Float, InstructionType::fSub);
		BASICOPERATION("*", BasicType::Float, InstructionType::fMul);
		BASICOPERATION("/", BasicType::Float, InstructionType::fDiv);
		BASICASSIGN("=", BasicType::Float, InstructionType::fSet);

		//float x int
		BASICOPERATIONEXT("+", (std::initializer_list<InstructionType>{ InstructionType::iTof0, InstructionType::fAdd }), BasicType::Float, BasicType::Int, BasicType::Float);
		BASICOPERATIONEXT("+", (std::initializer_list<InstructionType>{ InstructionType::iTof1, InstructionType::fAdd }), BasicType::Float, BasicType::Float, BasicType::Int);

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

	int BasicModule::getPrecedence(const std::string& _op)
	{
		for (auto& prec : m_precedence)
			if (prec.first == _op) return prec.second;

		return 1; // unknown operator or function takes precedence
	}
}