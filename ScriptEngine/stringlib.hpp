#pragma once

#include "module.hpp"

// primitive type string

namespace lang{

	struct StringModule : public NaReTi::Module
	{
		StringModule();

		// builds additional intrinsic functionality after the type has been compiled
		// todo: implement this completely in NaReTi
		void buildFunctions();
	};

	/* required intrinsics:
	 * constructor(), constructor(const str)
	 * destructor()
	 * =
	 * +
	 */
}