#pragma once

namespace par
{
	//not all of this are in use or even make sence in the current implementation
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
		//float arithmetic
		fAdd,
		fSub,
		fMul,
		fDiv,
		fMod,
		//logical operations
		Not,
		ShL,
		ShR,
		Set, //assign
		fSet,
		SetRef,
		fSetRef,
		//comparisation
		Eq,
		Ret,
		RetA, // return the accumulator; takes no param
		Call, // call subroutine; is inlined when the flag is set
		Push, // push anything on the virtual stack
		Scope, // a new scope "{}" inside the current one
		// type casts and which argument to take
		iTof0,
		iTof1,
		fToi
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
		Parameter() : parType(ParamType::None){};
		Parameter(int _val) : val(_val), parType(ParamType::Int){};
		Parameter(float _val) : valFloat(_val), parType(ParamType::Float){};
		Parameter(VarSymbol* _val) : ptr(_val), parType(ParamType::Ptr){};
		Parameter(par::Function* _val) : ptrFunc(_val), parType(ParamType::PtrFunc){};

		ParamType parType;
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