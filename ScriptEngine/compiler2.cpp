#include "compiler2.hpp"

namespace codeGen
{
	using namespace asmjit;
	using namespace par;

	Compiler::Compiler():
		m_assembler(&m_runtime),
		m_compiler(&m_assembler)
	{}

	// *************************************************** //

	void Compiler::compile(NaReTi::Module& _module)
	{
		for (auto& function : _module.m_functions)
		{
			compileFuction(*function);

			// Finalize the current function.
			m_compiler.endFunc();
			m_compiler.finalize();
			function->binary = m_assembler.make();
			m_assembler.reset(true);
			m_compiler.attach(&m_assembler);
		}
	}

	// *************************************************** //

	void Compiler::convertSignature(par::Function& _function)
	{
		FuncBuilderX& funcBuilder = _function.funcBuilder;
		for (int i = 0; i < _function.paramCount; ++i)
		{
			switch (_function.scope.m_variables[i].type.basic)
			{
			case BasicType::Int: funcBuilder.addArgT<int>(); break;
			case BasicType::Float: funcBuilder.addArgT<float>(); break;
			case BasicType::Complex: funcBuilder.addArgT<void*>(); break;
			default: funcBuilder.addArgT<int>();
			}
		}
		switch (_function.returnType.basic)
		{
		case BasicType::Int: funcBuilder.setRetT<int>(); break;
		case BasicType::Float: funcBuilder.setRetT<float>(); break;
		case BasicType::Complex: funcBuilder.setRetT<void*>(); break;
		default: funcBuilder.setRetT<int>();
		}

	}

	// *************************************************** //

	void Compiler::compileFuction(par::Function& _function)
	{
		m_function = &_function;

		//setup signature
		convertSignature(_function);
		m_compiler.addFunc(_function.funcBuilder);


		//setup registers

		m_anonymousVars.clear();
		m_anonymousVars.reserve(32); // make shure that no move will occure
		m_anonymousVars.push_back(m_compiler.newInt32("accumulator"));
		m_accumulator = &m_anonymousVars[0];

		m_anonymousFloats.clear();
		m_anonymousFloats.reserve(32);
		m_anonymousFloats.push_back(m_compiler.newXmmSs("fp0"));
		m_fp0 = &m_anonymousFloats[0];

		//create arguments and locals
		for (int i = 0; i < _function.scope.m_variables.size(); ++i)
		{
			asmjit::Var* varPtr;
			switch (_function.scope.m_variables[i].type.basic)
			{
			case BasicType::Int: 
				m_anonymousVars.push_back(m_compiler.newInt32(("arg" + std::to_string(i)).c_str()));
				varPtr = &m_anonymousVars.back();
				break;
			case BasicType::Float:
				m_anonymousFloats.push_back(m_compiler.newXmmSs(("arg" + std::to_string(i)).c_str()));
				varPtr = &m_anonymousFloats.back();
				break;
			}
			m_compiler.setArg(i, *varPtr);
			_function.scope.m_variables[i].compiledVar = varPtr;
		}
		//args are relevant through out the function
		m_usageState.varsInUse = m_anonymousVars.size();
		m_usageState.floatsInUse = m_anonymousFloats.size();

		//code
		compileCode(_function.scope);
		//a final return
//		m_compiler.ret();

	}

	// *************************************************** //

	void Compiler::compileCode(ASTCode& _node)
	{
		for (auto& subNode : _node)
		{
			switch (subNode->type)
			{
	//		case ASTType::BinOp:
	//			compileBinOp(*(ASTBinOp*)subNode);
				break;
			case ASTType::Call:
				compileCall(*(ASTCall*)subNode);
				break;
			case ASTType::Ret:
				compileRet(*(ASTReturn*)subNode);
				break;
			}
		}
	}

	// *************************************************** //

	void Compiler::compileCall(ASTCall& _node)
	{
		Function& func = *_node.function;

		std::vector< asmjit::Operand* > args; args.reserve(_node.args.size());

		UsageState preCallState = getUsageState();

		if (_node.function->bInline)
		{
			int i = 0;
			//manage args and put make shure that all are located in virtual registers
			for (auto& arg : _node.args)
			{
				switch (arg->type)
				{
				case ASTType::Leaf:
					ASTLeaf* leaf; leaf = (ASTLeaf*)arg;
					//immediate val
					if (leaf->parType == ParamType::Int)
					{
						X86GpVar& var = getUnusedVar();
						m_compiler.mov(var, Imm(leaf->val));
					}
					else if (leaf->parType == ParamType::Float)
					{
					//	m_compiler.
					}
					else if(leaf->parType == ParamType::Ptr)
					{
						args.emplace_back(((ASTLeaf*)arg)->ptr->compiledVar);
					}
					break;
				case ASTType::Call: 
					ASTCall* astCall = (ASTCall*)arg;
					if (astCall->function->returnType.basic == BasicType::Int)
					{
						X86GpVar& var = getUnusedVar();
						args.emplace_back(&var);
						compileCall(*astCall);
						m_compiler.mov(var, *m_accumulator);
					}
					else if (astCall->function->returnType.basic == BasicType::Float)
					{
						X86XmmVar& var = getUnusedFloat();
						args.emplace_back(&var);
						compileCall(*astCall);
						m_compiler.movss(var, *m_fp0);
					}
					break;
				}
				i++;
			}
			//code
			ASTBinOp& binOp = *(ASTBinOp*)_node.function->scope[0];

			//since the first operand of a binop is overwritten with the result copy the values first
			if (func.returnType.basic == BasicType::Float)
			{
				m_compiler.movss(*m_fp0, *(X86XmmVar*)args[0]);
				args[0] = m_fp0;
			}
			else
			{
				m_compiler.mov(*m_accumulator, *(asmjit::X86GpVar*)args[0]);
				args[0] = m_accumulator;
			}

			switch (binOp.instruction)
			{
			case InstructionType::Add:
				m_compiler.add(*(asmjit::X86GpVar*)args[0], *(asmjit::X86GpVar*)args[1]);
				break;
	/*		case InstructionType::Sub:
				m_compiler.sub(*args[0], *args[1]);
				break;
			case InstructionType::Mul:
				m_compiler.imul(*args[0], *args[1]);
				break;*/
			case InstructionType::fAdd:
				m_compiler.addss(*(X86XmmVar*)args[0], *(X86XmmVar*)args[1]);
				break;
			}

			//the result is already in ax or fp0
		} // end if inline
		else
		{
			m_compiler.mov(*m_accumulator, imm_ptr(func.binary));
			X86CallNode* call = m_compiler.call(*m_accumulator, func.funcBuilder);
			for (int i = 0; i < func.paramCount; ++i)
				call->_setArg(i, *args[0]);
		}

		setUsageState(preCallState);
	}

	// *************************************************** //

/*	void Compiler::compileBinOp(ASTBinOp& _node)
	{
		
		m_compiler.mov(m_accumulator, ((ASTLeaf*)_node.lOperand)->ptr->binVar);

		switch (_node.instruction)
		{
		case InstructionType::Add:
			m_compiler.add(m_accumulator, ((ASTLeaf*)_node.rOperand)->ptr->binVar);
			break;
		case InstructionType::Mul:
			m_compiler.imul(m_accumulator, ((ASTLeaf*)_node.rOperand)->ptr->binVar);
		}
	}*/

	// *************************************************** //

	void Compiler::compileRet(ASTReturn& _node)
	{
		if (_node.body->type == ASTType::Call) compileCall(*(ASTCall*)_node.body);
	//	else if (_node.body->type == ASTType::BinOp) compileBinOp(*(ASTBinOp*)_node.body);

		par::Type& returnType = (*(ASTCall*)_node.body).function->returnType;
		if (returnType.basic == Float)
			m_compiler.ret(*m_fp0);
		else
			m_compiler.ret(*m_accumulator);
	}

	// *************************************************** //

	asmjit::X86GpVar& Compiler::getUnusedVar()
	{
		if (m_anonymousVars.size() == m_usageState.varsInUse) m_anonymousVars.push_back(m_compiler.newInt32());

		return m_anonymousVars[m_usageState.varsInUse++];
	}

	// *************************************************** //

	asmjit::X86XmmVar& Compiler::getUnusedFloat()
	{
		if (m_anonymousFloats.size() == m_usageState.floatsInUse) m_anonymousFloats.push_back(m_compiler.newXmmSs());

		return m_anonymousFloats[m_usageState.floatsInUse++];
	}
}