#pragma once

#include <vector>
#include <memory>
#include "instruction.hpp"

#include "compiledsymbols.hpp"// for x86grpvar

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
		FlagBool, // bool in the form of eflags after a cmp instr
		IntRef,
		FloatRef,
		BoolRef,
		StringRef,
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

	struct ComplexType;

	struct TypeInfo
	{
		TypeInfo(ComplexType& _type, bool _isRef = false, bool _isConst = false) : type(_type), isReference(_isRef), isConst(_isConst){}
		ComplexType& type;
		bool isReference;
		bool isConst;

		bool operator== (TypeInfo& oth)
		{
			return (&type == &oth.type && isReference == oth.isReference && isConst == oth.isConst);
		}

		bool operator!= (TypeInfo& oth)
		{
			return !(*this == oth);
		}
	};

	struct VarSymbol : public Symbol, public codeGen::CVarSymbol
	{
		VarSymbol(const std::string& _name, TypeInfo& _typeInfo) :
			Symbol(_name),
			typeInfo(_typeInfo)
		{}

		VarSymbol& operator= (VarSymbol& oth)
		{
			name = oth.name;
//			typeInfo.type = oth.typeInfo.type;
//			typeInfo.isReference = oth.typeInfo.isReference;

			return *this;
		}

		TypeInfo typeInfo;
	};

	struct CodeScope
	{
		CodeScope() : m_parent(nullptr) {};

		std::vector< VarSymbol* > m_variables;

		VarSymbol* getVar(std::string& _name)
		{
			for (auto& var : m_variables)
				if (var->name == _name) return var;

			return m_parent ? m_parent->getVar(_name) : nullptr;
		}

		//the above level scope
		//module level functions have no parent
		CodeScope* m_parent; 
	};

	struct Function;

	struct ComplexType : public Type
	{
		ComplexType(){};
		ComplexType(const std::string& _name, BasicType _basicType = Complex) : Type(_name, _basicType)
		{}
		CodeScope scope;

		// available casts for this type
		std::vector< Function* > m_typeCasts;
		//offsets of the member vars
		//is set by the compiler
		std::vector < int > displacement;
		size_t size; // size in bytes

		int sizeOf() override
		{
			int size = 0;

	//		for (auto& member : scope.m_variables)
//				size += member.isReference ? 4 : member.type.sizeOf();

			return size;
		}
	};

	// Function symbol got moved to ast.hpp

}