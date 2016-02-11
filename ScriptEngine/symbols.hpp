#pragma once

#include <vector>
#include "instruction.hpp"

#include <asmjit.h> // for x86grpvar

namespace par{
	
	struct Symbol
	{
		Symbol() = default;
		Symbol(const std::string& _name) :
			name(_name)
		{
		}
		std::string name;
	};

	enum BasicType
	{
		Int,
		Float,
		Bool,
		String,
		Void, //when no type is given it is implicitly void
		Complex,
		Undefined // has to be linked first (currently not in use)
	};

	struct Type : public Symbol
	{
		Type(){};
		Type(const std::string& _name, BasicType _basic) : Symbol(_name), basic(_basic){};
		BasicType basic;

		virtual int sizeOf()
		{
			switch (basic)
			{
			case BasicType::Bool: return 1;
			case Int:
			case Float:
			case String:
				return 4; // references, int and float all are one dword
			}
		};
	};

	// ************************************************* //

	struct VarSymbol : public Symbol
	{
//		VarSymbol(){}; //todo: add better way to acuire atom type; use this here
		VarSymbol(const std::string& _name, Type& _type) :
			Symbol(_name),
			type(_type)
		{};

		VarSymbol& operator= (VarSymbol& oth)
		{
			name = oth.name;
			type = oth.type;
			isReference = oth.isReference;

			return *this;
		}

		Type& type;

		bool isReference;

		asmjit::X86GpVar binVar;
	};

	struct CodeScope
	{
		CodeScope() : m_parent(nullptr) {};

		std::vector< VarSymbol > m_variables;
//		std::vector < Instruction > m_instructions;

		VarSymbol* getVar(std::string& _name)
		{
			for (auto& var : m_variables)
				if (var.name == _name) return &var;

			return m_parent ? m_parent->getVar(_name) : nullptr;
		}

		//the above level scope
		//module level functions have no parent
		CodeScope* m_parent; 
	};

	struct ComplexType : public Type
	{
		ComplexType(){};
		ComplexType(const std::string& _name, BasicType _basicType = Complex) : Type(_name, _basicType)
		{}
		CodeScope scope;

		int sizeOf() override
		{
			int size = 0;

			for (auto& member : scope.m_variables)
				size += member.isReference ? 4 : member.type.sizeOf();

			return size;
		}
	};

	// Function symbol got moved to ast.hpp

}