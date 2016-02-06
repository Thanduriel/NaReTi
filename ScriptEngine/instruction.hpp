#pragma once

namespace par
{
	enum InstructionType
	{
		//set accumulator
		SetA,
		//arithmetical operations
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		//logical operations
		Not,
		ShL,
		ShR,
		Set, //assign
		Ret,
		RetA, // return the accumulator; takes no param
		Call, // call subroutine; is inlined when the flag is set
		Push, // push anything on the virtual stack
		Scope // a new scope "{}" inside the current one
	};

	//predeclarations
	struct VarSymbol;
	struct Function;

	enum class ParamType
	{
		Int,
		Float,
		Ptr,
		PtrFunc, // only for calls
		None // no param (only valid for operations with defined behaviour like ret)
	};

	/* Parameter
	 * A parameter for NaReTi byte code instructions.
	 * Can take an atomic type of data or a reference.
	 */
	struct Parameter
	{
		Parameter() : type(ParamType::None){};
		Parameter(int _val) : val(_val), type(ParamType::Int){};
		Parameter(float _val) : valFloat(_val), type(ParamType::Float){};
		Parameter(VarSymbol* _val) : ptr(_val), type(ParamType::Ptr){};
		Parameter(par::Function* _val) : ptrFunc(_val), type(ParamType::PtrFunc){};

		ParamType type;
		union
		{
			int val;
			float valFloat;
			VarSymbol* ptr;
			Function* ptrFunc;
		};
	};

	// stuff that needs to fit in here:
	// CodeScope
	// arithmetical operations with params
	// assignment op
	// conditionals
	/* **************************************
	 * Istruction
	 * A NaReTi byte code instruction
	 */
	struct Instruction 
	{
		Instruction(InstructionType _type, Parameter& _param) :
			type(_type),
			param(_param){};

		//constructor with arg forwarding (for the param)
		template< typename _T>
		Instruction(InstructionType _type, _T _param) :
			type(_type),
			param(_param){};

		InstructionType type;
		Parameter param;
	};
}