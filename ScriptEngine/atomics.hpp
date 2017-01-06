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

		par::ComplexType& getBasicType(par::BasicType _basicType);
		par::TypeInfo& getBasicTypeInfo(par::BasicType _basicType);
		int getPrecedence(const std::string& _op);

	private:
		void makeConstant(const std::string& _name, int _val);

		std::array< par::TypeInfo*, 6 > m_typeInfos;
		std::array< std::pair< std::string, int >, 23> m_precedence;
	};

	extern BasicModule* g_module;
//	extern BasicModule g_module;
}