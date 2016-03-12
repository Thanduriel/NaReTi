#include "compiler2.hpp"

namespace codeGen
{
	using namespace asmjit;
	using namespace par;

	Compiler::Compiler():
		m_assembler(&m_runtime),
		m_compiler(&m_assembler),
		m_isRefSet(false)
	{
		m_labelStack.reserve(64); // prevent moving
	}

	// *************************************************** //

	void Compiler::compile(NaReTi::Module& _module)
	{
		for (auto& type : _module.m_types)
		{
			compileType(*(ComplexType*)type.get());
		}
		for (auto& var : _module.m_text.m_variables)
		{
			compileHeapVar(var, _module.getAllocator());
		}
		for (auto& function : _module.m_functions)
		{
			compileFuction(*function);
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
		_type.size = currentOffset;
	}

	// *************************************************** //

	void Compiler::compileHeapVar(VarSymbol& _var, utils::StackAlloc& _allocator)
	{
		_var.ownership.rawPtr = _allocator.alloc(_var.typeInfo.type.size);
		_var.ownership.ownerType = OwnershipType::Heap;
	//	_var.typeInfo.isReference = true;
	}

	// *************************************************** //

	void Compiler::convertSignature(par::Function& _function)
	{
		FuncBuilderX& funcBuilder = _function.funcBuilder;
		for (int i = 0; i < _function.paramCount; ++i)
		{
			switch (_function.scope.m_variables[i].typeInfo.type.basic)
			{
			case BasicType::Int: funcBuilder.addArgT<int>(); break;
			case BasicType::Float: funcBuilder.addArgT<float>(); break;
			case BasicType::Complex: funcBuilder.addArgT<void*>(); break;
			default: funcBuilder.addArgT<int>();
			}
		}
		switch (_function.returnTypeInfo.type.basic)
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
		//externals are not compiled
		if (_function.bExternal) return;
		m_compiler.addFunc(_function.funcBuilder);


		//setup registers

		m_anonymousVars.clear();
		m_anonymousVars.reserve(32); // make sure that no move will occur
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
			VarSymbol& varSymbol = _function.scope.m_variables[i];
			if (varSymbol.typeInfo.isReference)
			{
				m_anonymousVars.push_back(m_compiler.newIntPtr(("arg" + std::to_string(i)).c_str()));
				varPtr = &m_anonymousVars.back();
				varSymbol.isPtr = true;
			}
			else
			{
				switch (varSymbol.typeInfo.type.basic)
				{
				case BasicType::Int:
					m_anonymousVars.push_back(m_compiler.newInt32(("arg" + std::to_string(i)).c_str()));
					varPtr = &m_anonymousVars.back();
					break;
				case BasicType::Float:
					m_anonymousFloats.push_back(m_compiler.newXmmSs(("arg" + std::to_string(i)).c_str()));
					varPtr = &m_anonymousFloats.back();
					break;
				case BasicType::Complex:
					m_anonymousVars.push_back(m_compiler.newIntPtr(("arg" + std::to_string(i)).c_str()));
					X86GpVar& gpVar = m_anonymousVars.back();
					varPtr = &gpVar;
					X86Mem mem = m_compiler.newStack(varSymbol.typeInfo.type.size, 4);
					m_compiler.lea(gpVar, mem);
					varSymbol.isPtr = true;
					break;
				}
			}
			
			_function.scope.m_variables[i].compiledVar = varPtr;
		}
		for (int i = 0; i < _function.paramCount; ++i)
			m_compiler.setArg(i, *_function.scope.m_variables[i].compiledVar);

		//imported heap vars
		for (auto& var : _function.m_importedVars)
		{
			m_anonymousVars.push_back(m_compiler.newIntPtr());
			X86GpVar& gpVar = m_anonymousVars.back();
			var->compiledVar = &gpVar;
			m_compiler.mov(gpVar, asmjit::imm_ptr(var->ownership.rawPtr));
		}

		//args are relevant through out the function
		m_usageState.varsInUse = m_anonymousVars.size();
		m_usageState.floatsInUse = m_anonymousFloats.size();

		//code
		compileCode(_function.scope);
		
		// Finalize the current function.
		m_compiler.endFunc();
		m_compiler.finalize();
		_function.binary = m_assembler.make();
		m_assembler.reset(true);
		m_compiler.attach(&m_assembler);

	}

	// *************************************************** //

	void Compiler::compileCode(ASTCode& _node)
	{
		for (auto& subNode : _node)
		{
			switch (subNode->type)
			{
			case ASTType::Code:
				compileCode(*(ASTCode*)subNode);
				break;
			case ASTType::Branch:
				compileBranch(*(ASTBranch*)subNode);
				break;
			case ASTType::Loop:
				compileLoop(*(ASTLoop*)subNode);
				break;
			case ASTType::Call:
				compileCall(*(ASTCall*)subNode);
				break;
			case ASTType::Ret:
				ASTReturn* retNode = (ASTReturn*)subNode;
				if (retNode->body->typeInfo->type.basic == BasicType::Float)
					compileRetF(*(ASTReturn*)subNode);
				else compileRet(*(ASTReturn*)subNode);
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

		auto begin = _node.args.begin();
		if (func.bHiddenParam)
		{
			//allocate the stack var and provide a reference as param
			X86GpVar& gpVar = getUnusedVar();
			X86Mem mem = m_compiler.newStack(func.scope.m_variables[0].typeInfo.type.size, 4);
			m_compiler.lea(gpVar, mem);
			args.emplace_back(&gpVar);
			//this arg is already managed
	//		begin++;
		}
		int i = 0;
		//manage args and put make sure that all are located in virtual registers
		for (; begin != _node.args.end(); ++begin)
		{
			auto& arg = *begin;
			switch (arg->type)
			{
			case ASTType::Leaf:
				ASTLeaf* leaf; leaf = (ASTLeaf*)arg;
				args.emplace_back(compileLeaf(*leaf));
				break;
			case ASTType::Member:
			{
				ASTMember& member = *(ASTMember*)arg;
				if (member.instance->type == ASTType::Call) compileCall(*(ASTCall*)member.instance);
				if (func.name == "=" && i == 0)
				{
					X86GpVar& var = getUnusedVar();
					args.emplace_back(&var);
					m_compiler.lea(var, getMemberAdr(*(ASTMember*)arg));
					m_isRefSet = true;
				}
				else
				{
					X86GpVar& var = getUnusedVar();
					args.emplace_back(&var);
					compileMemberLd(*(ASTMember*)arg, &var);
				}
				break;
			}
			case ASTType::Call:
				ASTCall* astCall = (ASTCall*)arg;
				if (astCall->function->returnTypeInfo.type.basic == BasicType::Int)
				{
					X86GpVar& var = getUnusedVar();
					args.emplace_back(&var);
					compileCall(*astCall);
					m_compiler.mov(var, *m_accumulator);
				}
				else if (astCall->function->returnTypeInfo.type.basic == BasicType::Float)
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

		if (_node.function->bInline)
		{
			//code

			// if types do not match, a typecast will move the data
			if (func.returnTypeInfo.type.basic == func.scope.m_variables[0].typeInfo.type.basic && !(func.name[0] == '='))
			{
				//since the first operand of a binop is overwritten with the result copy the values first
				if (func.returnTypeInfo.type.basic == BasicType::Float)
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
				call->_setArg(i, *args[i]);
			if (func.returnTypeInfo.type.basic == BasicType::Float)
				call->setRet(0, *m_fp0);
			else 
				call->setRet(0, *m_accumulator);
		}

		setUsageState(preCallState);
	}

	// *************************************************** //

	asmjit::Operand* Compiler::compileLeaf(par::ASTLeaf& _node)
	{
		//immediate val
		if (_node.parType == ParamType::Int)
		{
			X86GpVar& var = getUnusedVar();
			m_compiler.mov(var, Imm(_node.val));
			return &var;
		}
		else if (_node.parType == ParamType::Float)
		{
			//	m_compiler.
		}
		else if (_node.parType == ParamType::Ptr)
		{
			return ((ASTLeaf*)&_node)->ptr->compiledVar;
		}
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
		case Mod:
		{
			X86GpVar& dummy = getUnusedVar();
			m_compiler.xor_(dummy, dummy);
			m_compiler.idiv(dummy, *(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			m_compiler.mov(*(X86GpVar*)_args[0], dummy);
			break;
		}
		case Div:
		{
			X86GpVar& dummy = getUnusedVar();
			m_compiler.xor_(dummy, dummy);
			m_compiler.idiv(dummy, *(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		}
		case ShL:
			m_compiler.shl(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case ShR:
			m_compiler.shr(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case And:
			m_compiler.and_(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case Xor:
			m_compiler.xor_(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case Or:
			m_compiler.or_(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case InstructionType::Set:
			if (m_isRefSet) {
				m_compiler.mov(x86::dword_ptr(*(X86GpVar*)_args[0]), *(X86GpVar*)_args[1]); m_isRefSet = false;
			} else
				m_compiler.mov(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case Cmp:
			m_compiler.cmp(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case JNE:
			m_compiler.jne(m_labelStack.back());
			break;
		case JNL:
			m_compiler.jnl(m_labelStack.back());
			break;
		//float instructions
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
		asmjit::X86GpVar* var;
		if (_node.body->type == ASTType::Call)
		{
			compileCall(*(ASTCall*)_node.body);
			var = m_accumulator;
		}
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			compileMemberLd(member, m_accumulator);
			var = m_accumulator;
		}
		else if (_node.body->type == ASTType::Leaf)
		{
			var = (X86GpVar*)compileLeaf(*(ASTLeaf*)_node.body);
		}

		if (m_function->bHiddenParam)
		{
			X86GpVar& dest = *(X86GpVar*)m_function->scope.m_variables[0].compiledVar;
			compileMemCpy(dest, *var, m_function->returnTypeInfo.type.size);

			var = &dest;
		}
		m_compiler.ret(*var);
	}

	// *************************************************** //

	void Compiler::compileRetF(ASTReturn& _node)
	{
		X86XmmVar* var;
		if (_node.body->type == ASTType::Call)
		{
			compileCall(*(ASTCall*)_node.body);
			var = m_fp0;
		}
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			compileMemberLd(member, m_fp0);
			var = m_fp0;
		}
		else if (_node.body->type == ASTType::Leaf)
		{
			var = (X86XmmVar*)compileLeaf(*(ASTLeaf*)_node.body);
		}

		m_compiler.ret(*var);
	}

	// *************************************************** //

	void Compiler::compileMemCpy(X86GpVar& _dst, X86GpVar& _src, size_t _size)
	{
		X86GpVar& tmp = getUnusedVar();
		size_t chunks = _size / 4;
		for (size_t i = 0; i < chunks; ++i)
		{
			m_compiler.mov(tmp, x86::dword_ptr(_src, i*4));
			m_compiler.mov(x86::dword_ptr(_dst, i*4), tmp);
		}
/*		X86GpVar& cnt = getUnusedVar();

		Label L_Loop(m_compiler);

		m_compiler.mov(cnt, imm(_size));

		m_compiler.bind(L_Loop);                                // Bind the loop label here.

		X86GpVar& tmp = getUnusedVar();              // Copy a single dword (4 bytes).
		m_compiler.mov(tmp, x86::dword_ptr(_src));
		m_compiler.mov(x86::dword_ptr(_dst), tmp);

		m_compiler.add(_src, 4);                                 // Increment dst/src pointers.
		m_compiler.add(_dst, 4);

		m_compiler.dec(cnt);                                    // Loop until cnt isn't zero.
		m_compiler.jnz(L_Loop);*/
	}

	// *************************************************** //

	X86Mem Compiler::getMemberAdr(ASTMember& _node)
	{
		X86GpVar* gpVar;
		switch (_node.instance->type)
		{
		case ASTType::Leaf:
		{
			//can only be a pointer
			ASTLeaf& leaf = *(ASTLeaf*)_node.instance;
			gpVar = (X86GpVar*)leaf.ptr->compiledVar;
			break;
		}
		case ASTType::Call:
			gpVar = m_accumulator;
			break;
		}

		ComplexType& type = _node.instance->typeInfo->type;
		return x86::dword_ptr(*gpVar, type.displacement[_node.index]);
	}

	// *************************************************** //

	void Compiler::compileMemberLd(ASTMember& _node, asmjit::Operand* _destination)
	{
		ComplexType& type = _node.instance->typeInfo->type;
		auto adr = getMemberAdr(_node);
		if (type.scope.m_variables[_node.index].typeInfo.type.basic == BasicType::Float)
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

	void Compiler::compileBranch(ASTBranch& _node)
	{
		Label end(m_compiler);
		m_labelStack.emplace_back(m_compiler);
		Label& elseBranch = m_labelStack.back();
		
		compileCondExp(*(ASTCall*)_node.condition);
	//	m_compiler.jnz(elseBranch);
		compileCode(*_node.ifBody);
		m_compiler.jmp(end);
		m_compiler.bind(elseBranch);

		if (_node.elseBody) compileCode(*_node.elseBody);

		m_compiler.bind(end);
		m_labelStack.pop_back();
	}

	// *************************************************** //

	void Compiler::compileLoop(ASTLoop& _node)
	{
		Label begin(m_compiler);

		m_labelStack.emplace_back(m_compiler);
		Label& end = m_labelStack.back();

		m_compiler.bind(begin);
		compileCondExp(*(ASTCall*)_node.condition);
		compileCode(*_node.body);
		m_compiler.jmp(begin);

		m_compiler.bind(end);
	}

	// *************************************************** //

	void Compiler::compileCondExp(ASTCall& _node)
	{
		if (_node.function->name == "||")
		{
			m_labelStack.emplace_back(m_compiler);
			Label& end = m_labelStack.back();
			Label postOr(m_compiler);

			compileCondExp(*(ASTCall*)_node.args[0]);
			m_compiler.jmp(postOr); //on success the Or statement is true
			m_compiler.bind(end); // the second Or condition
			m_labelStack.pop_back();

			compileCondExp(*(ASTCall*)_node.args[1]);
			m_compiler.bind(postOr);
		}
		else if (_node.function->name == "&&")
		{
			compileCondExp(*(ASTCall*)_node.args[0]);
			compileCondExp(*(ASTCall*)_node.args[1]);
		}
		else compileCall(_node);
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