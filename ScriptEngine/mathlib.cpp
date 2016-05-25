#include "mathlib.hpp"
#include "defmacros.hpp"
#include "atomics.hpp"
#include <math.h>

namespace lang{

	using namespace par;
	using namespace std;

	MathModule::MathModule():
		NaReTi::Module("math")
	{
		UNARYOPERATION("sqrt", TypeInfo(g_module->getBasicType(Float)), InstructionType::Sqrt);
//		UNARYOPERATION("abs", TypeInfo(g_module->getBasicType(Float)), InstructionType::Sqrt);

//		UNARYOPERATION("sin", TypeInfo(g_module->getBasicType(Float)), InstructionType::Sqrt);
//		UNARYOPERATION("cos", TypeInfo(g_module->getBasicType(Float)), InstructionType::Sqrt);

		//pow(float, float)
		m_functions.emplace_back(new Function("'", TypeInfo(g_module->getBasicType(Float))));
		Function& powFunc = *m_functions.back();
		powFunc.scope.m_variables.push_back(m_allocator.construct<VarSymbol>("_base", TypeInfo(g_module->getBasicType(Float))));
		powFunc.scope.m_variables.push_back(m_allocator.construct<VarSymbol>("_exp", TypeInfo(g_module->getBasicType(Float))));
		powFunc.paramCount = 2;
		powFunc.bExternal = true;
		linkExternal("'", static_cast<float(*)(float,float)>(pow));
	}
}