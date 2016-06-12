#include "mathlib.hpp"
#include "defmacros.hpp"
#include "atomics.hpp"
#include <math.h>
#include "logger.hpp"
#include "ast.hpp"

namespace lang{

	using namespace par;
	using namespace std;

	MathModule::MathModule():
		NaReTi::Module("math")
	{
		UNARYOPERATION("sqrt", TypeInfo(g_module->getBasicType(Float)), InstructionType::Sqrt);
	}

	void MathModule::linkExternals()
	{
		bool fail;
		//cast are necessary to determine the right overload
		fail = !linkExternal("'", static_cast<float(*)(float, float)>(pow));
		fail |= !linkExternal("sinp", static_cast<float(*)(float)>(sin));
		fail |= !linkExternal("cosp", static_cast<float(*)(float)>(cos));
		fail |= !linkExternal("tanp", static_cast<float(*)(float)>(tan));

		fail |= !linkExternal("sinh", static_cast<float(*)(float)>(sinh));
		fail |= !linkExternal("cosh", static_cast<float(*)(float)>(cosh));
		fail |= !linkExternal("tanh", static_cast<float(*)(float)>(tanh));

		if (fail) logging::log(logging::Error, "Could not link standard math functions.");
	}
}