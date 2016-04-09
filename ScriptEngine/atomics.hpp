#pragma once
#include "module.hpp"
#include <asmjit.h>
#include <array>
#include <functional>

namespace lang
{

	// the module that is always included by default
	// providing basic types, intrinsics (float and int operations)
	struct BasicModule: public NaReTi::Module
	{
		BasicModule(asmjit::JitRuntime& _runtime);

		par::ComplexType& getBasicType(par::BasicType _basicType);
		int getPrecedence(const std::string& _op);

	private:
		std::array< std::pair< std::string, int >, 22> m_precedence;
	};

	extern BasicModule* g_module;
//	extern BasicModule g_module;
}