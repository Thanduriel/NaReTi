#pragma once

#include <asmjit.h>

namespace codeGen{
	/* Structures that hold the compiled form of symbols or structures required to compile them 
	 * in other modules.
	 */
	struct CFunction
	{
		CFunction() : binary(nullptr){}

		//the compiled version (the callee should know its signature)
		void* binary;
		asmjit::FuncSignatureX funcBuilder; //signature needed to compile calls
	};

	enum class OwnershipType
	{
		Stack,
		Heap,
		Module 
	};

	struct Ownership
	{
		Ownership(OwnershipType _ownerType = OwnershipType::Stack) : ownerType(_ownerType){};
		OwnershipType ownerType;
		void* rawPtr;
	};

	struct CVarSymbol
	{
		CVarSymbol() : isSubstituted(false), isPtr(false), compiledVar(nullptr){}
		asmjit::Operand* compiledVar;
		Ownership ownership;
		bool isPtr; //< Is internally implemented as pointer. compiledVar then contains a address.
		bool isSubstituted; //< Is not required to be allocated.
	};
}