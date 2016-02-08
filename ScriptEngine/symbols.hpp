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

	struct Function : public Symbol
	{
		// a binary function of the structure T x T -> T
		Function(const std::string& _name, Type& _type, InstructionType _instr) : Function(_name, _type)
		{
			bInline = true;
			paramCount = 2;

			scope.m_variables.emplace_back("0", _type);
			scope.m_variables.emplace_back("1", _type);

			scope.m_instructions.emplace_back(InstructionType::SetA, &scope.m_variables[0]);
			scope.m_instructions.emplace_back(_instr, &scope.m_variables[1]);
		};

		Function(const std::string& _name, Type& _type) : Symbol(_name), returnType(_type){};

		Type& returnType;
		CodeScope scope;
		int paramCount; //< amount of params this function expects
		//flags
		bool bInline;

		//the compiled version (the callee should know its signiture)
		void* binary;
	};

}