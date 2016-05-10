#include "compiler2.hpp"
#include "ptr_stuff.hpp"
#include "logger.hpp"

#include <assert.h>
#include <time.h>

namespace codeGen
{
	using namespace asmjit;
	using namespace par;

#ifdef	_M_IX86
#define PTRSIZE 4
#endif
#ifdef _M_AMD64
#define PTRSIZE 8
#endif


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
		clock_t beginClock = clock();

		for (auto& type : _module.m_types)
		{
			compileType(*(ComplexType*)type.get());
		}
		for (auto& var : _module.m_text.m_variables)
		{
			compileHeapVar(*var, _module.getAllocator());
		}
		compileModuleInit(_module);

		for (auto& function : _module.m_functions)
		{
			compileFuction(*function);
		}

		clock_t endClock = clock();
		logging::log(logging::Info1, "Compiled \"" + _module.m_name + "\" in " + std::to_string(double(endClock - beginClock) / CLOCKS_PER_SEC) + "sec");
	}

	// *************************************************** //

	void Compiler::release(NaReTi::Module& _module)
	{
		for (auto& var : _module.m_text.m_variables)
		{
			m_runtime.release(var->ownership.rawPtr);
		}
		for (auto& function : _module.m_functions)
		{
			m_runtime.release(function->binary);
		}
	}

	// *************************************************** //

	void Compiler::compileType(ComplexType& _type)
	{
		int currentOffset = 0;
		_type.alignment = 16; // if any stack var has a different alignment float vars wont work properly
		_type.displacement.reserve(_type.scope.m_variables.size());
		for (auto& member : _type.scope.m_variables)
		{
			_type.displacement.push_back(currentOffset);
			currentOffset += member->typeInfo.isReference ? PTRSIZE : member->typeInfo.type.size;

//			if (member->typeInfo.type.basic == BasicType::Float && !member->typeInfo.isReference)
//				_type.alignment = 16;
		}
		_type.size = currentOffset;
	}

	// *************************************************** //

	void Compiler::compileHeapVar(VarSymbol& _var, utils::DetorAlloc& _allocator)
	{
		_var.ownership.rawPtr = m_runtime.getMemMgr()->alloc(_var.typeInfo.type.size);//_allocator.alloc(_var.typeInfo.type.size);
		_var.ownership.ownerType = OwnershipType::Module;
		_var.isPtr = true; 
		_var.typeInfo.isReference = true;  // enforce reference assignment
	}

	// *************************************************** //

	void Compiler::compileModuleInit(NaReTi::Module& _module)
	{
		if (_module.m_text.size() == 0) return;
		
		m_compiler.addFunc(FuncBuilder0<void>());
		resetRegisters();
		compileCode(_module.m_text);

		m_compiler.endFunc();
		m_compiler.finalize();
		void* ptr = m_assembler.make();
		((NaReTi::basicFunc*)ptr)();
		m_assembler.reset(true);
		m_compiler.attach(&m_assembler);
		m_runtime.release(ptr); // this function wont be needed anymore
	}

	// *************************************************** //

	void Compiler::convertSignature(par::Function& _function)
	{
		FuncBuilderX& funcBuilder = _function.funcBuilder;
		for (int i = 0; i < _function.paramCount; ++i)
		{
			switch (_function.scope.m_variables[i]->typeInfo.type.basic)
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
		default: funcBuilder.setRetT<void>();
		}

	}

	// *************************************************** //

	void Compiler::allocVar(VarSymbol& _sym)
	{
		asmjit::Var* varPtr;

		if (_sym.typeInfo.isReference)
		{
			varPtr = &getUnusedVar();
			_sym.isPtr = true;
		}
		else if (_sym.typeInfo.isArray)
		{
			_sym.isPtr = true;
			varPtr = allocStackVar(_sym.typeInfo.type, _sym.typeInfo.arraySize);
		}
		else
		{
			switch (_sym.typeInfo.type.basic)
			{
			case BasicType::Int:
				varPtr = &getUnusedVar32();
				break;
			case BasicType::Float:
				varPtr = &getUnusedFloat();
				break;
			case BasicType::Complex:
				_sym.isPtr = true;
				varPtr = allocStackVar(_sym.typeInfo.type);
				break;
			}
		}

		_sym.compiledVar = varPtr;

	}

	// *************************************************** //

	X86GpVar* Compiler::allocStackVar(ComplexType& _type, int _count)
	{
		X86GpVar& gpVar = getUnusedVar();
		X86Mem mem = m_compiler.newStack(_type.size * _count, _type.alignment); // if floats are used  -> operations with xmm(128) register

		m_compiler.lea(gpVar, mem);

		return &gpVar;
	}

	// *************************************************** //

	void Compiler::compileFuction(par::Function& _function)
	{
		m_function = &_function;

		//setup signature
		convertSignature(_function);
		//externals are not compiled
		//but still require a signature to be called
		if (_function.bExternal) return;
		m_compiler.addFunc(_function.funcBuilder);


		//setup registers
		resetRegisters();

		std::vector< utils::PtrReset > binVarLocations; binVarLocations.reserve(_function.scope.m_variables.size());

		//create arguments and locals
		for (int i = 0; i < _function.scope.m_variables.size(); ++i)
		{
			VarSymbol& varSymbol = *_function.scope.m_variables[i];
			if (varSymbol.isSubstituted) continue;

			allocVar(varSymbol);

			binVarLocations.emplace_back(&varSymbol.compiledVar);
		}
		for (int i = 0; i < _function.paramCount; ++i)
			m_compiler.setArg(i, *_function.scope.m_variables[i]->compiledVar);

		//code
		compileCode(_function.scope);
		
		// Finalize the current function.
		m_compiler.endFunc();
		m_compiler.finalize();
		_function.binary = m_assembler.make();
		m_assembler.reset(false); // todo: performance/mem test
		m_compiler.attach(&m_assembler);

	}

	// *************************************************** //

	void Compiler::compileCode(ASTCode& _node)
	{
		UsageStateLock lock(m_usageState);

		//imported heap vars
		for (auto& var : _node.m_importedVars)
		{
			X86GpVar& gpVar = getUnusedVar();
			var->compiledVar = &gpVar;
			m_compiler.mov(gpVar, asmjit::imm_ptr(var->ownership.rawPtr));
		}

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
				if (retNode->body->typeInfo->type.basic == BasicType::Float && !retNode->body->typeInfo->isReference)
					compileRetF(*(ASTReturn*)subNode);
				else compileRet(*(ASTReturn*)subNode);
				break;
			}
		}
	}

	// *************************************************** //

	Var* Compiler::compileCall(ASTCall& _node, asmjit::Var* _dest)
	{
		Function& func = *_node.function;

		bool indirect = false; // < use indirect addressing operations

		std::vector< asmjit::Var* > args; args.reserve(_node.args.size());
		std::vector< utils::PtrReset > binVarLocations; binVarLocations.reserve(func.scope.m_variables.size() - _node.args.size() + 1);

		//return values are part of the caller's scope
		if (func.bHiddenParam)
		{
			if (_node.returnSub) // optimization
			{
				_dest = _node.returnSub->compiledVar;
				args.push_back(_node.returnSub->compiledVar);
			}
			else
			{
				//allocate the stack var and provide a reference as param
				func.scope.m_variables[0]->compiledVar = (Var*)allocStackVar(func.scope.m_variables[0]->typeInfo.type);
				binVarLocations.emplace_back(&func.scope.m_variables[0]->compiledVar);
				args.push_back(func.scope.m_variables[0]->compiledVar);
			}
		}
		if (!_dest) _dest = getUnusedVarAuto(func.returnTypeInfo);

		UsageStateLock lock(m_usageState);

		int i = 0; //identify the first operand
		//make sure that all arguments are located in virtual registers
		for (auto& arg : _node.args)
		{
		//	auto& arg = *begin;
			switch (arg->type)
			{
			case ASTType::Leaf:
				ASTLeaf* leaf; leaf = (ASTLeaf*)arg;
				args.push_back(compileLeaf(*leaf, &indirect));
				break;
			case ASTType::String:
			{
				X86GpVar& var = getUnusedVar();
				args.push_back(&var);
				m_compiler.mov(var, imm_ptr(((ASTUnlinkedSym*)arg)->name.c_str()));
				break;
			}
			case ASTType::Member:
			{
				ASTMember& member = *(ASTMember*)arg;
				X86GpVar* baseVar = compileMemberAdr(member);
	
				if (func.name == "=" && i == 0)
				{
					X86GpVar* var = &getUnusedVar();
					args.push_back(var);
					m_compiler.lea(*var, getMemberAdr(*(ASTMember*)arg, *baseVar));
					indirect = true;
				}
				else
				{
					Var* var = getUnusedVarAuto(*member.typeInfo);
					args.push_back(var);
					if (var->getVarType() == 15) // xmm var
						compileMemberLdF(*(ASTMember*)arg, *baseVar, *(X86XmmVar*)var);
					else
						compileMemberLd(*(ASTMember*)arg, *baseVar, *(X86GpVar*)var);
				}
				break;
			}
			case ASTType::Call:
			{
				ASTCall* astCall = (ASTCall*)arg;
				args.push_back(compileCall(*astCall));
				break;
			}
			} // end switch
			if (arg->typeInfo->isReference) indirect = true;
			i++;
		}

		if (_node.function->bInline)
		{
			if (_node.function->bIntrinsic)
			{
				switch (_node.function->intrinsicType)
				{
				case Function::Assignment:
					if (indirect) m_isRefSet = true;
					_dest = args[0]; // assignments return their left site operand
					break;
				case Function::BinOp:
					//since the first operand of a binop is overwritten with the result copy the values first
					if (_dest->getVarType() == 15)
					{
						m_compiler.movss(*(X86XmmVar*)_dest, *(X86XmmVar*)args[0]);
					}
					else
					{
						m_compiler.mov(*(X86GpVar*)_dest, *(X86GpVar*)args[0]);
					}
					args[0] = _dest;
					break;
				case Function::TypeCast:
					args.push_back(_dest);
					break;
				case Function::Compare: break;
				default: break;
				}

				for (auto& node : _node.function->scope)
				{
					ASTOp& op = *(ASTOp*)node;
					compileOp(op.instruction, args);
				}
			}
			else // general purpose inline
			{
				for (int i = 0; i < args.size(); ++i)
					func.scope.m_variables[i]->compiledVar = args[i];
				for (int i = func.paramCount; i < func.scope.m_variables.size(); ++i)
				{
					if (!func.scope.m_variables[i]->compiledVar)
					{
						
						allocVar(*func.scope.m_variables[i]);
						binVarLocations.emplace_back(&func.scope.m_variables[i]->compiledVar);
					}
				}
				m_retDstStack.push_back(_dest);
				compileCode(_node.function->scope);
				m_retDstStack.pop_back();
			}
		} // end if inline
		else
		{
			X86GpVar& var = getUnusedVar();
			m_compiler.mov(var, imm_ptr(func.binary));
			X86CallNode* call = m_compiler.call(var, func.funcBuilder);
			for (int i = 0; i < func.paramCount; ++i)
				call->_setArg(i, *args[i]);
			if(_dest) call->setRet(0, *_dest);
		}

		if (func.name == "breakpoint")
			int uidae = 2l;

		return _dest;
	}

	// *************************************************** //

	asmjit::Var* Compiler::compileLeaf(par::ASTLeaf& _node, bool* _indirect)
	{
		//immediate val
		if (_node.parType == ParamType::Int)
		{
			X86GpVar& var = getUnusedVar32();
			m_compiler.mov(var, Imm(_node.val));
			return &var;
		}
		else if (_node.parType == ParamType::Float)
		{
			X86XmmVar& var = getUnusedFloat();
			m_compiler.movss(var, m_compiler.newFloatConst(0, _node.valFloat));
			return &var;
		}
		else if (_node.parType == ParamType::Ptr)
		{
			if (_indirect && _node.ptr->isPtr) *_indirect = true;
			return _node.ptr->compiledVar;
		}
		return nullptr;
	}

	// *************************************************** //

	void Compiler::compileOp(par::InstructionType _instr, std::vector< asmjit::Var* >& _args)
	{
		
		switch (_instr)
		{
		case InstructionType::Inc:
			m_compiler.inc(*(X86GpVar*)_args[0]);
			break;
		case InstructionType::Dec:
			m_compiler.dec(*(X86GpVar*)_args[0]);
			break;
		case InstructionType::Neg:
			m_compiler.mov(*(X86GpVar*)_args[1], *(X86GpVar*)_args[0]); //val is copied to not change the original and to have the result in the right destination
			m_compiler.neg(*(X86GpVar*)_args[1]);
			break;
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
		case InstructionType::fSet:
			if (m_isRefSet) {
				m_compiler.movss(x86::dword_ptr(*(X86GpVar*)_args[0]), *(X86XmmVar*)_args[1]); m_isRefSet = false;
			}
			else
				m_compiler.movss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;
		case InstructionType::Ld:
			m_compiler.mov(*(X86GpVar*)_args[1], x86::dword_ptr(*(X86GpVar*)_args[0]));
			break;
		case InstructionType::fLd:
			m_compiler.movss(*(X86XmmVar*)_args[1], x86::dword_ptr(*(X86GpVar*)_args[0]));
			break;
		case InstructionType::LdO:
			m_compiler.lea(*(X86GpVar*)_args[2], x86::dword_ptr(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1], 2)); //2
			break;
		case Cmp:
			m_compiler.cmp(*(X86GpVar*)_args[0], *(X86GpVar*)_args[1]);
			break;
		case CmpZ:
			m_compiler.cmp(*(X86GpVar*)_args[0], imm(0));
			break;
		case fCmp:
			m_compiler.comiss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;
		case JE:
			m_compiler.je(m_labelStack.back());
			break;
		case JNE:
			m_compiler.jne(m_labelStack.back());
			break;
		case JNL:
			m_compiler.jnl(m_labelStack.back());
			break;
		case JL:
			m_compiler.jl(m_labelStack.back());
			break;
		case JNG:
			m_compiler.jng(m_labelStack.back());
			break;
		case JG:
			m_compiler.jg(m_labelStack.back());
			break;
		case JNB:
			m_compiler.jnb(m_labelStack.back());
			break;
		case JB:
			m_compiler.jb(m_labelStack.back());
			break;
		case JNA:
			m_compiler.jna(m_labelStack.back());
			break;
		case JA:
			m_compiler.ja(m_labelStack.back());
			break;
		//float instructions
		case InstructionType::fAdd:
			m_compiler.addss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;
		case fSub:
			m_compiler.subss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;
		case fMul:
			m_compiler.mulss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;
		case fDiv:
			m_compiler.divss(*(X86XmmVar*)_args[0], *(X86XmmVar*)_args[1]);
			break;

		//typecasts
		case InstructionType::iTof:
			m_compiler.cvtsi2ss(*(X86XmmVar*)_args[1], *(X86GpVar*)_args[0]);
			break;
		case InstructionType::fToi:
			m_compiler.cvtss2si(*(X86GpVar*)_args[1], *(X86XmmVar*)_args[0]);
			break;
		}
	}

	// *************************************************** //

	void Compiler::compileRet(ASTReturn& _node)
	{
		UsageStateLock lock(m_usageState);

		asmjit::X86GpVar* var;
		if (_node.body->type == ASTType::Call)
		{
			var = (X86GpVar*)compileCall(*(ASTCall*)_node.body);
		}
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			var = &getUnusedVar();
			compileMemberLd(member, *compileMemberAdr(member), *var);
		}
		else if (_node.body->type == ASTType::Leaf)
		{
			var = (X86GpVar*)compileLeaf(*(ASTLeaf*)_node.body);
		}

		if (m_function->bHiddenParam)
		{
			X86GpVar& dest = *(X86GpVar*)m_function->scope.m_variables[0]->compiledVar;
			//due to substitution the value can already be in the destination
			if(&dest != var) compileMemCpy(dest, *var, m_function->returnTypeInfo.type.size);

			var = &dest;
		}

		if (m_retDstStack.size())
		{
			if (var != m_retDstStack.back()) m_compiler.mov(*(X86GpVar*)m_retDstStack.back(), *var);
		}
		else
			m_compiler.ret(*var);
	}

	// *************************************************** //

	void Compiler::compileRetF(ASTReturn& _node)
	{
		UsageStateLock lock(m_usageState);

		X86XmmVar* var;
		if (_node.body->type == ASTType::Call)
		{
			var = (X86XmmVar*)compileCall(*(ASTCall*)_node.body);
		}
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			var = &getUnusedFloat();
			compileMemberLdF(member, *compileMemberAdr(member), *var);
		}
		else if (_node.body->type == ASTType::Leaf)
		{
			var = (X86XmmVar*)compileLeaf(*(ASTLeaf*)_node.body);
		}

		if (m_retDstStack.size())
		{
			if (var != m_retDstStack.back()) m_compiler.movss(*(X86XmmVar*)m_retDstStack.back(), *var);
		}
		else
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
	}

	// *************************************************** //

	X86GpVar* Compiler::compileMemberAdr(par::ASTMember& _node)
	{
		X86GpVar* baseVar;
		if (_node.instance->type == ASTType::Call)
			baseVar = (X86GpVar*)compileCall(*(ASTCall*)_node.instance); // take a var that has a higher lifespan that is still valid in getMemberAdr
		else
		{
			ASTLeaf& leaf = *(ASTLeaf*)_node.instance;
			baseVar = (X86GpVar*)leaf.ptr->compiledVar;
		}

		return baseVar;
	}

	// *************************************************** //

	X86Mem Compiler::getMemberAdr(ASTMember& _node, X86GpVar& _var)
	{
	/*	X86GpVar* gpVar;
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
		}*/

		ComplexType& type = _node.instance->typeInfo->type;
		return x86::dword_ptr(_var, type.displacement[_node.index]);//dword
	}

	// *************************************************** //

	void Compiler::compileMemberLd(ASTMember& _node, X86GpVar& _var, X86GpVar& _destination)
	{
		auto adr = getMemberAdr(_node, _var);

		m_compiler.mov(_destination, adr);
	}

	// *************************************************** //

	void Compiler::compileMemberLdF(ASTMember& _node, X86GpVar& _var, X86XmmVar& _destination)
	{
		auto adr = getMemberAdr(_node, _var);
		
		m_compiler.movss(_destination, adr);
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
		if (m_anonymousVars.size() == m_usageState.varsInUse) m_anonymousVars.push_back(m_compiler.newIntPtr());

		return m_anonymousVars[m_usageState.varsInUse++];
	}

	// *************************************************** //

	asmjit::X86GpVar& Compiler::getUnusedVar32()
	{
		//on x86 just one size of variables has to be managed
#ifdef	_M_IX86
		return getUnusedVar();
#else
		if (m_anonymousVars32.size() == m_usageState.vars32InUse) m_anonymousVars32.push_back(m_compiler.newInt32());

		return m_anonymousVars32[m_usageState.vars32InUse++];
#endif
	}

	// *************************************************** //

	asmjit::X86XmmVar& Compiler::getUnusedFloat()
	{
		if (m_anonymousFloats.size() == m_usageState.floatsInUse) m_anonymousFloats.push_back(m_compiler.newXmmSs());

		return m_anonymousFloats[m_usageState.floatsInUse++];
	}

	// *************************************************** //

	asmjit::Var* Compiler::getUnusedVarAuto(TypeInfo& _typeInfo)
	{
		if (_typeInfo.isReference || _typeInfo.type.basic == BasicType::Complex) //complex is always a reference
			return &getUnusedVar();
		else if (_typeInfo.type.basic == BasicType::Float)
			return &getUnusedFloat();
		else if (_typeInfo.type.size == 4)
			return &getUnusedVar32();
		else if (_typeInfo.type.basic == BasicType::Void)
			return nullptr;

		assert(0);//should never happen
		return &getUnusedVar(); 
	}

	// *************************************************** //

	void Compiler::resetRegisters()
	{
		m_anonymousVars.clear();
		m_anonymousVars.reserve(32); // make sure that no move will occur

		m_anonymousVars32.clear();
		m_anonymousVars32.reserve(32);

		m_anonymousFloats.clear();
		m_anonymousFloats.reserve(32); //32

		m_usageState.varsInUse = 0;
		m_usageState.floatsInUse = 0;
		m_usageState.vars32InUse = 0;
	}
}