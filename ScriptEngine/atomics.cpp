#include "atomics.hpp"
#include "symbols.hpp"

#define BASICASSIGN(Name, T, Instr) m_functions.emplace_back(new Function( m_allocator, Name, *m_types[ T ], Instr)); m_functions.back()->scope.m_variables[0]->typeInfo.isConst = false; m_functions.back()->intrinsicType = Function::Assignment;
#define BASICOPERATION(X, Y, Z) m_functions.emplace_back(new Function( m_allocator, X, *m_types[ Y ], Z));
#define BASICOPERATIONEXT(Name, InstrList, T0, T1, T2) m_functions.emplace_back(new Function( m_allocator, Name, InstrList, *m_types[ T0 ], m_types[ T1 ].get(), m_types[ T2 ].get()));
#define BASICCAST(Instr, T0, T1) T0.type.typeCasts.emplace_back(new Function( m_allocator, "", Instr, T1, T0));

namespace lang
{
	using namespace par;
	using namespace std;

	typedef std::initializer_list<InstructionType> InstrList;

	BasicModule* g_module;
	//make a this call linkable to an external 
	asmjit::JitRuntime* __runtime;
	void* __allocBoundFunc(int _size)
	{
		return __runtime->getMemMgr()->alloc(_size);
	}

	void __freeBoundFunc(void* _ptr)
	{
		__runtime->getMemMgr()->release(_ptr);
	}

	BasicModule::BasicModule(asmjit::JitRuntime& _runtime) :
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
		__runtime = &_runtime;
		g_module = this;

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
		//assignment to address is currently decided by the compiler
		// this is only to make sure that no typecast is necessary
		//todo: better concept for ref assign
		BASICASSIGN("=", BasicType::Int, InstructionType::Set); m_functions.back()->scope.m_variables[0]->typeInfo.isReference = true;
		BASICOPERATION("<<", BasicType::Int, InstructionType::ShL);
		BASICOPERATION(">>", BasicType::Int, InstructionType::ShR);
		BASICOPERATION("&", BasicType::Int, InstructionType::And);
		BASICOPERATION("^", BasicType::Int, InstructionType::Xor);
		BASICOPERATION("|", BasicType::Int, InstructionType::Or);
		//comparison
		BASICOPERATIONEXT("==", (InstrList{ Cmp, JNE }), FlagBool, Int, Int);
		BASICOPERATIONEXT("!=", (InstrList{ Cmp, JE }), FlagBool, Int, Int);
		BASICOPERATIONEXT("<", (InstrList{ Cmp, JNL }), FlagBool, Int, Int);
		BASICOPERATIONEXT(">", (InstrList{ Cmp, JNG }), FlagBool, Int, Int);
		BASICOPERATIONEXT(">=", (InstrList{ Cmp, JL }), FlagBool, Int, Int);
		BASICOPERATIONEXT("<=", (InstrList{ Cmp, JG }), FlagBool, Int, Int);

		//float -------------------------------------------------------
		BASICOPERATION("+", Float, InstructionType::fAdd);
		BASICOPERATION("-", Float, InstructionType::fSub);
		BASICOPERATION("*", Float, InstructionType::fMul);
		BASICOPERATION("/", Float, InstructionType::fDiv);
		BASICASSIGN("=", Float, InstructionType::fSet);
		BASICASSIGN("=", Float, InstructionType::fSet); m_functions.back()->scope.m_variables[0]->typeInfo.isReference = true;

		//comparison
		BASICOPERATIONEXT("==", (InstrList{ fCmp, JNE }), FlagBool, Float, Float);
		BASICOPERATIONEXT("!=", (InstrList{ fCmp, JE }), FlagBool, Float, Float);
		BASICOPERATIONEXT("<", (InstrList{ fCmp, JNB }), FlagBool, Float, Float);
		BASICOPERATIONEXT(">", (InstrList{ fCmp, JNA }), FlagBool, Float, Float);
		BASICOPERATIONEXT(">=", (InstrList{ fCmp, JB }), FlagBool, Float, Float);
		BASICOPERATIONEXT("<=", (InstrList{ fCmp, JA }), FlagBool, Float, Float);


		//typecasts
		BASICCAST(InstructionType::iTof, TypeInfo(*m_types[Int], false, true), TypeInfo(*m_types[Float]));
		BASICCAST(InstructionType::fToi, TypeInfo(*m_types[Float], false, true), TypeInfo(*m_types[Int]));
		BASICCAST(InstructionType::Ld, TypeInfo(*m_types[Int], true, true), TypeInfo(*m_types[Int]));
		BASICCAST(InstructionType::fLd, TypeInfo(*m_types[Float], true, true), TypeInfo(*m_types[Float]));

		//build function for dynamic allocation
		m_functions.emplace_back(new Function("alloc", TypeInfo(*m_types[BasicType::Void], true)));
		Function& allocFunc = *m_functions.back();
		allocFunc.scope.m_variables.push_back(m_allocator.construct<VarSymbol>("size", TypeInfo(*m_types[Int])));
		allocFunc.paramCount = 1;
		allocFunc.bExternal = true;
		linkExternal("alloc", &__allocBoundFunc);
		//and free
		m_functions.emplace_back(new Function("free", TypeInfo(*m_types[BasicType::Void])));
		Function& freeFunc = *m_functions.back();
		allocFunc.scope.m_variables.push_back(m_allocator.construct<VarSymbol>("ptr", TypeInfo(*m_types[Void], true)));
		allocFunc.paramCount = 1;
		allocFunc.bExternal = true;
		linkExternal("free", &__freeBoundFunc);

		//global constants
		makeConstant("true", 1);
		makeConstant("false", 0);
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

	void BasicModule::makeConstant(const std::string& _name, int _val)
	{
		m_text.m_variables.push_back(m_allocator.construct<par::VarSymbol>(_name, par::TypeInfo(*m_types[BasicType::Int], true, true)));
		VarSymbol& var = *m_text.m_variables.back();
		var.ownership.rawPtr = m_allocator.alloc(var.typeInfo.type.size);
		var.ownership.ownerType = codeGen::OwnershipType::Heap;
		var.isPtr = true;
		var.typeInfo.isReference = true;

		//write value
		*(int*)var.ownership.rawPtr = _val;
	}
}