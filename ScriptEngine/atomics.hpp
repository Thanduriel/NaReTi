#pragma once
#include "module.hpp"

namespace lang
{

	// the module that is always included by default
	struct BasicModule: public NaReTi::Module
	{
		BasicModule();
	};
	extern BasicModule g_module;
}