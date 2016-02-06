#include "compiler2.hpp"

namespace codeGen
{
	using namespace asmjit;

	Compiler::Compiler():
		m_assembler(&m_runtime),
		m_compiler(&m_assembler)
	{}

	// *************************************************** //

	void Compiler::compile(NaReTi::Module& _module)
	{
		for (auto& function : _module.m_functions)
		{
			compileFuction(function);

			// Finalize the current function.
			m_compiler.endFunc();
			m_compiler.finalize();

			function.binary = m_assembler.make();
		}
	}

	// *************************************************** //

	void Compiler::compileFuction(par::Function& _function)
	{
		m_function = &_function;

		m_accumulator = m_compiler.newInt32("accumulator");
		//setup signature
		FuncBuilderX funcBuilder;
		for (int i = 0; i < _function.paramCount; ++i)
			funcBuilder.addArgT<int>();
		funcBuilder.setRetT<int>();

		m_compiler.addFunc(funcBuilder);
		//arguments
		for (int i = 0; i < _function.paramCount; ++i)
		{
			_function.scope.m_variables[i].binVar = m_compiler.newInt32(("arg" + std::to_string(i)).c_str());
			//m_vars.emplace_back(m_compiler.newInt32(("arg" + std::to_string(i)).c_str()));
			m_compiler.setArg(i, _function.scope.m_variables[i].binVar);
		}

		//code
//		m_compiler.add(m_vars[0], m_vars[1]);
//		m_compiler.ret(m_vars[0]);
		size_t i = 0;
		while (i < _function.scope.m_instructions.size())
		{
			if (_function.scope.m_instructions[i].type == par::InstructionType::Call && _function.scope.m_instructions[i].param.ptrFunc->bInline)
				i = inlineFunction(i);
			else i++;
		}
		for (auto& instr : _function.scope.m_instructions)
		{
			translateInstruction(instr);
		}
		//a final return
//		m_compiler.ret();

	}

	// *************************************************** //

	void Compiler::translateInstruction(par::Instruction& _instruction)
	{
		switch (_instruction.type)
		{
		case par::InstructionType::SetA:
			m_compiler.mov(m_accumulator, _instruction.param.ptr->binVar);
			break;
		case par::InstructionType::Add:
			m_compiler.add(m_accumulator, _instruction.param.ptr->binVar);
			break;
		case par::InstructionType::Ret:
			if (_instruction.param.type == par::ParamType::Ptr)
				m_compiler.ret(_instruction.param.ptr->binVar);
	//		else if (_instruction.param.type == par::ParamType::Int)
	//			m_compiler.ret(_instruction.param.val);
		}
	}

	// *************************************************** //

	size_t Compiler::inlineFunction(size_t _id)
	{
		auto& instructions = m_function->scope.m_instructions;
		auto& calledFunc = *instructions[_id].param.ptrFunc;
		
		size_t firstInstr = _id - calledFunc.paramCount;
		//save the used arguments
		std::vector<par::Parameter> args; args.reserve(calledFunc.paramCount);
		for (size_t i = firstInstr; i < firstInstr + calledFunc.paramCount; ++i)
		{
			args.push_back(instructions[i].param);
		}

		//resize
		//the functions instructions minus the call and the pushes
		int extraSpace = calledFunc.scope.m_instructions.size() - 1 - calledFunc.paramCount;
		if (extraSpace < 0) instructions.erase(instructions.begin() + firstInstr, instructions.begin() + firstInstr - extraSpace);
		else if (extraSpace > 0) instructions.insert(instructions.begin() + firstInstr, extraSpace, instructions[0]);

		//insert the actual instructions from the called function
		for (auto& instr : calledFunc.scope.m_instructions)
		{
			instructions[firstInstr].type = instr.type;
			instructions[firstInstr].param = args[instr.param.ptr->name[0] - '0'];
			firstInstr++;
		}

		return firstInstr;
	}

}