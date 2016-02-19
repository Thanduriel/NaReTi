#pragma once

#include <asmjit.h>

namespace codeGen{
	/* Structures that hold the compiled form of symbols or structures required to compile them 
	 * in other modules.
	 */
	struct CFunction
	{
		//the compiled version (the callee should know its signiture)
		void* binary;
		asmjit::FuncBuilderX funcBuilder; //signature needed to compile calls
	};

	enum class CVarType
	{
		Mem, //memory
		Reg, //register
		Float // float register
	};

	struct CVarSymbol
	{
		CVarType cType;
	};

	template < typename _T, CVarType _VarType>
	struct CompilerVar : public CVarSymbol
	{
		CompilerVar(_T&& _var) : asmjitVar(_var){ cType = _VarType; };
		_T asmjitVar;
	};

	typedef CompilerVar<asmjit::X86GpVar, CVarType::Reg> IntReg;
	typedef CompilerVar<asmjit::X86XmmVar, CVarType::Float> FloatReg;
	typedef CompilerVar<asmjit::X86MmVar, CVarType::Mem> MemPtr;

	typedef CompilerVar<asmjit::X86GpVar*, CVarType::Reg> IntRegPtr;
	typedef CompilerVar<asmjit::X86XmmVar*, CVarType::Float> FloatRegPtr;
	typedef CompilerVar<asmjit::X86MmVar*, CVarType::Mem> MemPtrPtr;
}