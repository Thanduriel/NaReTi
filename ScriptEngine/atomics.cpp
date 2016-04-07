#include "atomics.hpp"
#include "symbols.hpp"

#define BASICASSIGN(Name, T, Instr) m_functions.emplace_back(new Function( m_allocator, Name, *m_types[ T ], Instr)); m_functions.back()->scope.m_variables[0]->typeInfo.isConst = false; m_functions.back()->intrinsicType = Function::Assignment;
#define BASICOPERATION(X, Y, Z) m_functions.emplace_back(new Function( m_allocator, X, *m_types[ Y ], Z));
#define BASICOPERATIONEXT(Name, InstrList, T0, T1, T2) m_functions.emplace_back(new Function( m_allocator, Name, InstrList, *m_types[ T0 ], m_types[ T1 ].get(), m_types[ T2 ].get()));
#define BASICCAST(Instr, T0, T1) m_types[ T0 ]->typeCasts.emplace_back(new Function( m_allocator, "", Instr, *m_types[ T1 ], *m_types[ T0 ]));

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
			pair<string, int>(">", 8),
			pair<string, int>("<=", 8),
			pair<string, int>(">=", 8),
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
		BASICOPERATIONEXT("!=", (std::initializer_list<InstructionType>{ Cmp, JE }), BasicType::FlagBool, BasicType::Int, BasicType::Int);
		BASICOPERATIONEXT("<", (std::initializer_list<InstructionType>{ Cmp, JNL }), BasicType::FlagBool, BasicType::Int, BasicType::Int);
		BASICOPERATIONEXT(">", (std::initializer_list<InstructionType>{ Cmp, JNG }), BasicType::FlagBool, BasicType::Int, BasicType::Int);
		BASICOPERATIONEXT(">=", (std::initializer_list<InstructionType>{ Cmp, JL }), BasicType::FlagBool, BasicType::Int, BasicType::Int);
		BASICOPERATIONEXT("<=", (std::initializer_list<InstructionType>{ Cmp, JG }), BasicType::FlagBool, BasicType::Int, BasicType::Int);

		//float -------------------------------------------------------
		BASICOPERATION("+", BasicType::Float, InstructionType::fAdd);
		BASICOPERATION("-", BasicType::Float, InstructionType::fSub);
		BASICOPERATION("*", BasicType::Float, InstructionType::fMul);
		BASICOPERATION("/", BasicType::Float, InstructionType::fDiv);
		BASICASSIGN("=", BasicType::Float, InstructionType::fSet);

		//comparison
		BASICOPERATIONEXT("==", (std::initializer_list<InstructionType>{ fCmp, JNE }), BasicType::FlagBool, BasicType::Float, BasicType::Float);
		BASICOPERATIONEXT("!=", (std::initializer_list<InstructionType>{ fCmp, JE }), BasicType::FlagBool, BasicType::Float, BasicType::Float);
		BASICOPERATIONEXT("<", (std::initializer_list<InstructionType>{ fCmp, JNB }), BasicType::FlagBool, BasicType::Float, BasicType::Float);
		BASICOPERATIONEXT(">", (std::initializer_list<InstructionType>{ fCmp, JNA }), BasicType::FlagBool, BasicType::Float, BasicType::Float);
		BASICOPERATIONEXT(">=", (std::initializer_list<InstructionType>{ fCmp, JB }), BasicType::FlagBool, BasicType::Float, BasicType::Float);
		BASICOPERATIONEXT("<=", (std::initializer_list<InstructionType>{ fCmp, JA }), BasicType::FlagBool, BasicType::Float, BasicType::Float);


		//typecasts
		BASICCAST(InstructionType::iTof, BasicType::Int, BasicType::Float);
		BASICCAST(InstructionType::fToi, BasicType::Float, BasicType::Int);


		//global constants todo: make them useful
		m_text.m_variables.push_back(m_allocator.construct<par::VarSymbol>("true", par::TypeInfo(*m_types[BasicType::Int])));
		m_text.m_variables.push_back(m_allocator.construct<par::VarSymbol>("false", par::TypeInfo(*m_types[BasicType::Int])));
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