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
		for (auto& type : _module.m_types)
		{
			compileType(*(ComplexType*)type.get());
		}
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

	void Compiler::compileType(ComplexType& _type)
	{
		int currentOffset = 0;
		_type.displacement.reserve(_type.scope.m_variables.size());
		for (auto& member : _type.scope.m_variables)
		{
			_type.displacement.push_back(currentOffset);
			currentOffset += 4;
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
			if (_function.scope.m_variables[i].isReference)
			{
				m_anonymousVars.push_back(m_compiler.newIntPtr(("arg" + std::to_string(i)).c_str()));
				varPtr = &m_anonymousVars.back();
			}
			else
			{
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
	//			compileBinOp(*(ASTOp*)subNode);
	//			break;
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
						args.emplace_back(&var);
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
				case ASTType::Member:
//					args.emplace_back(&compileMemberLd(*(ASTMember*)arg));
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

			// if types do not match, a typecast will move the data
			if (func.returnType.basic == func.scope.m_variables[0].type.basic)
			{
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
			}

			for (auto& node : _node.function->scope)
			{
				ASTOp& op = *(ASTOp*)node;
				compileOp(op.instruction, args);
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

	void Compiler::compileOp(par::InstructionType _instr, std::vector< asmjit::Operand* >& _args)
	{
		
		switch (_instr)
		{
		case InstructionType::Add:
			m_compiler.add(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case InstructionType::Sub:
			m_compiler.sub(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
			case InstructionType::Mul:
				m_compiler.imul(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case InstructionType::fAdd:
			m_compiler.addss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;
		case InstructionType::iTof0:
			X86XmmVar& var = *m_fp0;
			m_compiler.cvtsi2ss(var, *(X86GpVar*)_args[0]);
			_args[0] = &var;
			break;
		}
	}

	// *************************************************** //

	void Compiler::compileRet(ASTReturn& _node)
	{
		if (_node.body->type == ASTType::Call) compileCall(*(ASTCall*)_node.body);
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			compileMemberLd(member, member.expType->basic == BasicType::Float ? (asmjit::Operand*)m_fp0 : (asmjit::Operand*)m_accumulator);
		}
	//	else if (_node.body->type == ASTType::BinOp) compileBinOp(*(ASTOp*)_node.body);

		par::Type& returnType = *_node.body->expType;//(*(ASTCall*)_node.body).function->returnType
		if (returnType.basic == Float)
			m_compiler.ret(*m_fp0);
		else
			m_compiler.ret(*m_accumulator);
	}

	// *************************************************** //

	void Compiler::compileMemberLd(ASTMember& _node, asmjit::Operand* _destination)
	{
		X86GpVar* gpVar;
		switch (_node.instance->type)
		{
		case ASTType::Leaf:
			//can only be a pointer
			ASTLeaf& leaf = *(ASTLeaf*)_node.instance;
			gpVar = (X86GpVar*)leaf.ptr->compiledVar;
//			case ASTType::
		}
		ComplexType& type = *_node.instance->expType;
		auto adr = x86::dword_ptr(*gpVar, type.displacement[_node.index]);
		if (type.scope.m_variables[_node.index].type.basic == BasicType::Float)
		{
			X86XmmVar& var = *(X86XmmVar*)_destination;
			m_compiler.movss(var, adr);
		}
		else
		{
			X86GpVar& var = *(X86GpVar*)_destination;
			m_compiler.mov(var, adr);
		}
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