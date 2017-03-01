#pragma once
#include "module.hpp"

#include <array>
#include <functional>

namespace asmjit{
	class JitRuntime;
}

namespace par{
	enum BasicType;
}

namespace lang
{

	// the module that is always included by default
	// providing basic types, intrinsics (float and int operations)
	struct BasicModule : public NaReTi::Module
	{
		BasicModule(asmjit::JitRuntime& _runtime);
		~BasicModule();
		void initConstants();

		const par::ComplexType& getBasicType(par::BasicType _basicType) const;
		par::ComplexType& getBasicType(par::BasicType _basicType);
		const par::TypeInfo& getBasicTypeInfo(par::BasicType _basicType) const;
		int getPrecedence(const std::string& _op);

		par::Function* tryBasicCast(const par::TypeInfo& _lhs, const par::TypeInfo& _rhs);
	private:
		void makeConstant(const std::string& _name, int _val);

		std::array< par::TypeInfo*, 8 > m_typeInfos;
		std::array< std::pair< std::string, int >, 23> m_precedence;

		std::unique_ptr<par::Function> m_dummyCast;
	};

	extern BasicModule* g_module;
//	extern BasicModule g_module;
}