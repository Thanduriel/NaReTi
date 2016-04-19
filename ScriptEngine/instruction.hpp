#pragma once

namespace par
{
	// instructions used to build intrinsics
	//not all of this are in use or even make sense in the current implementation
	enum InstructionType
	{
		Nop,
		//arithmetical operations
		Add,
		Sub,
		Mul,
		Div,
		Mod,
		Neg,
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
		Xor,
		And,
		Or,
		//assign
		Set, 
		fSet,
		//load value from adr
		Ld,
		fLd,
		LdO, // load offset adr
		//comparisation
		fCmp,
		Cmp,
		CmpZ, //compare to 0
		JE,
		JNE,
		JNL,
		JL,
		JNB,
		JB,
		JNG,
		JG,
		JNA,
		JA,
		Ret,
		Call, // call subroutine
		// type casts and which argument to take
		iTof,
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