#include "stringlib.hpp"
#include "generics.hpp"
#include "atomics.hpp"

namespace lang{

	using namespace par;

	StringModule::StringModule():
		Module("string")
	{
	}

	void StringModule::buildFunctions()
	{
		ComplexType& type = *getType("string");

		type.typeCasts.emplace_back(new Function(getAllocator(), "", InstructionType::Nop,
			TypeInfo(g_module->getBasicType(BasicType::String), true), TypeInfo(type, true)));
		type.typeCasts.back()->intrinsicType = Function::StaticCast;
	}
}