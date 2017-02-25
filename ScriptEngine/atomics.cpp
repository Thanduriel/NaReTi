#include <asmjit.h>

#include "atomics.hpp"
#include "symbols.hpp"
#include "defmacros.hpp"
#include "symbols.hpp"
#include "ast.hpp"

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
		void* ptr = malloc(_size);
		return ptr;// __runtime->getMemMgr()->alloc(_size);
	}

	void __freeBoundFunc(void* _ptr)
	{
		free(_ptr);//__runtime->getMemMgr()->release(_ptr);
	}

	BasicModule::BasicModule(asmjit::JitRuntime& _runtime) :
		Module(""),
		m_precedence( { {
			pair<string, int>("++", 2),
			pair<string, int>("--", 2),
			pair<string, int>(".", 2),
	//		pair<string, int>("-", 3), //actually unary minus but can not be differentiated
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
			pair<string, int>("=", 15),
			pair<string, int>(":=", 15)
			} }
		)
	{
		__runtime = &_runtime;
		g_module = this;

		m_types.resize(BasicType::Count);
		//basic types
		m_types[BasicType::Int] = new ComplexType("int", BasicType::Int); m_types[BasicType::Int]->size = 4; m_types[BasicType::Int]->alignment = 16;
		m_types[BasicType::Float] = new ComplexType("float", BasicType::Float); m_types[BasicType::Float]->size = 4;
		m_types[BasicType::String] = new ComplexType("string", BasicType::String);
		m_types[BasicType::Void] = new ComplexType("void", BasicType::Void);
		m_types[BasicType::Bool] = new ComplexType("bool", BasicType::Bool); m_types[BasicType::Bool]->size = 4;
		m_types[BasicType::FlagBool] = new ComplexType("flagBool", BasicType::FlagBool); m_types[BasicType::FlagBool]->size = 4;
		m_types[BasicType::Undefined] = new ComplexType("undefined", BasicType::Undefined); m_types[BasicType::Undefined]->size = 4;


		for (size_t i = 0; i < m_types.size(); ++i)
			m_typeInfos[i] = new TypeInfo(*m_types[i]);
		//void type can only be a ptr
		m_typeInfos[BasicType::Void]->isReference = true;
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
		UNARYOPERATION("-", TypeInfo(*m_types[Int]), InstructionType::Neg);
		//type is assignment because the original value is changed
		UNARYOPERATION("++", TypeInfo(*m_types[Int]), InstructionType::Inc); m_functions.back()->intrinsicType = Function::Assignment;
		UNARYOPERATION("--", TypeInfo(*m_types[Int]), InstructionType::Dec); m_functions.back()->intrinsicType = Function::Assignment;
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
		UNARYOPERATION("-", TypeInfo(*m_types[Float]), InstructionType::fNeg);
		//comparison
		BASICOPERATIONEXT("==", (InstrList{ fCmp, JNE }), FlagBool, Float, Float);
		BASICOPERATIONEXT("!=", (InstrList{ fCmp, JE }), FlagBool, Float, Float);
		BASICOPERATIONEXT("<", (InstrList{ fCmp, JNB }), FlagBool, Float, Float);
		BASICOPERATIONEXT(">", (InstrList{ fCmp, JNA }), FlagBool, Float, Float);
		BASICOPERATIONEXT(">=", (InstrList{ fCmp, JB }), FlagBool, Float, Float);
		BASICOPERATIONEXT("<=", (InstrList{ fCmp, JA }), FlagBool, Float, Float);

		//references
		BASICASSIGN("=", Void, InstructionType::Set); m_functions.back()->scope.m_variables[0]->typeInfo.isReference = true; m_functions.back()->scope.m_variables[1]->typeInfo.isReference = true;

		//typecasts
		BASICCAST(InstructionType::iTof, TypeInfo(*m_types[Int], false, true), TypeInfo(*m_types[Float]));	// int -> float
		BASICCAST(InstructionType::fToi, TypeInfo(*m_types[Float], false, true), TypeInfo(*m_types[Int]));	// float -> int
		BASICCAST(InstructionType::Ld, TypeInfo(*m_types[Int], true, true), TypeInfo(*m_types[Int]));		// int& -> int
		BASICCAST(InstructionType::fLd, TypeInfo(*m_types[Float], true, true), TypeInfo(*m_types[Float]));	// float& -> float
		//this cast requires two instructions
		BASICCAST(InstructionType::CmpZ, TypeInfo(*m_types[Int], false, true), TypeInfo(*m_types[FlagBool]));// int -> bool
		m_types[Int]->typeCasts.back()->scope.emplace_back(m_allocator.construct<ASTOp>(InstructionType::JE));

		//int -> void ref
		m_functions.emplace_back(new Function("voidRef", TypeInfo(*m_types[BasicType::Void], true)));
		Function& voidFunc = *m_functions.back();
		voidFunc.scope.m_variables.push_back(m_allocator.construct<VarSymbol>("i", TypeInfo(*m_types[Int])));
		voidFunc.paramCount = 1;
		

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
		freeFunc.scope.m_variables.push_back(m_allocator.construct<VarSymbol>("ptr", TypeInfo(*m_types[Void], true)));
		freeFunc.paramCount = 1;
		freeFunc.bExternal = true;
		linkExternal("free", &__freeBoundFunc);

		//global constants
		makeConstant("true", 1);
		makeConstant("false", 0);

		m_dummyCast = std::make_unique<Function>(m_allocator, std::string("dummy_cast"), InstructionType::Nop, TypeInfo(*m_types[Undefined]), TypeInfo(*m_types[Undefined]));
		m_dummyCast->intrinsicType = Function::StaticCast;
	}

	BasicModule::~BasicModule()
	{
		for (auto ptr : m_typeInfos)
			delete ptr;
	}

	void BasicModule::initConstants()
	{
		*(int*)m_text->m_variables[0]->ownership.rawPtr = 1;
		*(int*)m_text->m_variables[1]->ownership.rawPtr = 0;
	}

	// ******************************************************* //
	const par::ComplexType& BasicModule::getBasicType(par::BasicType _basicType) const
	{
		return *m_types[_basicType];
	}

	par::ComplexType& BasicModule::getBasicType(par::BasicType _basicType)
	{
		return *m_types[_basicType];
	}

	const TypeInfo& BasicModule::getBasicTypeInfo(BasicType _basicType) const
	{
		return *m_typeInfos[_basicType];
	}

	// ******************************************************* //
	int BasicModule::getPrecedence(const std::string& _op)
	{
		for (auto& prec : m_precedence)
			if (prec.first == _op) return prec.second;

		return 1; // unknown operator or function takes precedence
	}

	// ******************************************************* //
	par::Function* BasicModule::tryBasicCast(const par::TypeInfo& _lhs, const par::TypeInfo& _rhs)
	{
		//const copy to non const
		if (&_lhs.type == &_rhs.type && !_rhs.isReference)
			return m_dummyCast.get();
		return nullptr;
	}

	// ******************************************************* //
	void BasicModule::makeConstant(const std::string& _name, int _val)
	{
		m_text->m_variables.push_back(m_allocator.construct<par::VarSymbol>(_name, par::TypeInfo(*m_types[BasicType::Int], false, true)));
	/*	VarSymbol& var = *m_text->m_variables.back();
		var.ownership.rawPtr = m_allocator.alloc(var.typeInfo.type.size);
		var.ownership.ownerType = codeGen::OwnershipType::Heap;
		var.isPtr = true;
		var.typeInfo.isReference = true;

		//write value
		*(int*)var.ownership.rawPtr = _val;*/
	}

	// ******************************************************* //
	void BasicModule::buildStringType()
	{
		ComplexType& type = getBasicType(BasicType::String);
		// constructors:
		
	}
}