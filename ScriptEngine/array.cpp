#include "array.hpp"
#include "atomics.hpp"

namespace lang{
	using namespace par;
	using namespace std;
	void ArrayTypeGen::build(par::ComplexType& _type)
	{
		m_currentModule = createModule("arrayType_" + _type.name);
		if (!m_currentModule) return;

		m_currentType = &_type;
	}

	void ArrayTypeGen::buildAppend()
	{
		m_currentModule->m_functions.emplace_back(new Function("+=", TypeInfo(g_module->getBasicType(BasicType::Void))));
	}

	void ArrayTypeGen::buildElemAccess()
	{
		m_currentModule->m_functions.emplace_back(new Function("[]", TypeInfo(*m_currentType, true)));
	}

	void ArrayTypeGen::buildConst(ComplexType& _type)
	{
	//	createModule("carrayType_" + _type.name);
		m_currentModule = lang::g_module;

		m_currentModule->m_functions.emplace_back(new Function("[]", TypeInfo(*m_currentType, true, false, false)));
		Function& func = *m_currentModule->m_functions.back();
		auto& alloc = m_currentModule->getAllocator();
		func.scope.m_variables.push_back(alloc.construct<VarSymbol>("_0", TypeInfo(*m_currentType, true, true, true)));
		func.scope.m_variables.push_back(alloc.construct<VarSymbol>("_1", TypeInfo(lang::g_module->getBasicType(BasicType::Int))));
		func.paramCount = 2;
		func.scope.emplace_back(m_currentModule->getAllocator().construct<ASTOp>(InstructionType::LdO));
	}

	NaReTi::Module* ArrayTypeGen::createModule(string& _name)
	{
		//already exists
		for (auto& mod : m_modules)
			if (mod->m_name == _name) return nullptr;

		m_modules.emplace_back(new NaReTi::Module(_name));
		return m_modules.back().get();
	}
}