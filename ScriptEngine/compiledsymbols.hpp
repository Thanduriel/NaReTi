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

	enum class OwnershipType
	{
		Stack,
		Heap
	};

	struct Ownership
	{
		Ownership(OwnershipType _ownerType = OwnershipType::Stack) : ownerType(_ownerType){};
		OwnershipType ownerType;
		void* rawPtr;
	};

	struct CVarSymbol
	{
		asmjit::Var* compiledVar;
		Ownership ownership;
//		X86Mem rawPtr;
	};
}