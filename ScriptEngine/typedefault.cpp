#include "typedefault.hpp"
#include "ast.hpp"
#include "atomics.hpp"

using namespace par;
using namespace NaReTi;

namespace lang{

	void TypeDefaultGen::buildElemAccess(par::ComplexType& _type, NaReTi::Module& _module)
	{
		_module.m_functions.emplace_back(new Function("[]", TypeInfo(_type, true, false, false)));
		Function& func = *_module.m_functions.back();
		auto& alloc = _module.getAllocator();
		func.scope.m_variables.push_back(alloc.construct<VarSymbol>("_0", TypeInfo(_type)));
		func.scope.m_variables.push_back(alloc.construct<VarSymbol>("_1", TypeInfo(lang::g_module->getBasicType(BasicType::Int))));
		func.paramCount = 2;
		func.scope.emplace_back(alloc.construct<ASTOp>(InstructionType::LdO));
		func.bIntrinsic = true;
		func.bInline = true;
		func.intrinsicType = Function::TypeCast; //makes the compiler use _dest as arg
	}

	void TypeDefaultGen::buildDefaultAssignment(par::ComplexType& _type, NaReTi::Module& _module, par::ModuleLibrary& _lib)
	{
		TypeInfo typeInfo = TypeInfo(_type, true);
		utils::DetorAlloc& allocator = _module.getAllocator();

		// generate default assignment
		_module.m_functions.emplace_back(new Function("=", typeInfo));
		Function& func = *_module.m_functions.back();
		func.returnTypeInfo.isReference = true;

		// =(Type& slf, Type& oth)
		func.scope.m_variables.reserve(2); // prevent moves
		func.scope.m_variables.emplace_back(allocator.construct<VarSymbol>("slf", typeInfo));
		VarSymbol& slf = *func.scope.m_variables.back();
		ASTLeafSym& slfInst = *allocator.construct<ASTLeafSym>(&slf);
		slfInst.typeInfo = &slf.typeInfo;
		func.scope.m_variables.emplace_back(allocator.construct<VarSymbol>("oth", typeInfo));
		VarSymbol& oth = *func.scope.m_variables.back();
		ASTLeafSym& othInst = *allocator.construct<ASTLeafSym>(&oth);
		othInst.typeInfo = &oth.typeInfo;

		func.paramCount = 2;

		//code scope
		for (int i = 0; i < (int)_type.scope.m_variables.size(); ++i)
		{
			//slf.x = oth.x
			VarSymbol& member = *_type.scope.m_variables[i];
			ASTMember& memberSlf = *allocator.construct<ASTMember>();
			memberSlf.instance = &slfInst;
			memberSlf.index = i;
			memberSlf.typeInfo = &member.typeInfo;
			ASTMember& memberOth = *allocator.construct<ASTMember>();
			memberOth.instance = &othInst;
			memberOth.index = i;
			memberOth.typeInfo = &member.typeInfo;
			ASTCall& call = *allocator.construct<ASTCall>();
			call.args.push_back(&memberSlf);
			call.args.push_back(&memberOth);
			call.name = "=";
			call.typeInfo = &func.returnTypeInfo;
			call.function = _lib.getFunction(call.name, call.args.begin(), call.args.end(), m_funcQuery);
			func.scope.push_back(&call);
		}
	}

}