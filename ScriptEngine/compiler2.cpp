#include "compiler2.hpp"
#include "ptr_stuff.hpp"
#include "logger.hpp"
#include "ast.hpp"
#include "symbols.hpp"

#include <assert.h>
#include <time.h>

namespace codeGen{
	using namespace asmjit;
	using namespace par;

#ifdef	_M_IX86
#define PTRSIZE 4
#endif
#ifdef _M_AMD64
#define PTRSIZE 8
#endif


	Compiler::Compiler():
		m_runtime(),
		m_codeHolder(),
		m_compiler((m_codeHolder.init(m_runtime.getCodeInfo()), &m_codeHolder))
	{
		m_labelStack.reserve(64); // prevent moving
		m_retDstStack.reserve(64);
	}

	// *************************************************** //

	void Compiler::compile(NaReTi::Module& _module)
	{
		clock_t beginClock = clock();

		for (auto& type : _module.m_types)
		{
			compileType(*(ComplexType*)type);
		}
		for (auto& var : _module.m_text->m_variables)
		{
			compileHeapVar(*var);
		}
		for (auto& function : _module.m_functions)
		{
			compileFuction(*function);
		}
		//do this last so that anything defined in this module can be used
		compileModuleInit(_module);

		_module.setChanged(false); // is compiled up to date

		clock_t endClock = clock();
		LOG(Info1, "Compiled \"" << _module.m_name << "\" in " << (double(endClock - beginClock) / CLOCKS_PER_SEC) << "sec");
	}

	// *************************************************** //

	void Compiler::release(NaReTi::Module& _module)
	{
		//globals
		for (auto& var : _module.m_text->m_variables)
		{
			m_runtime.release(var->ownership.rawPtr);
		}
		//functions
		for (auto& function : _module.m_functions)
		{
			m_runtime.release(function->binary);
		}
	}

	// *************************************************** //

	void Compiler::compileType(ComplexType& _type)
	{
		int currentOffset = _type.size; //primitive types already have a size
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

	void Compiler::compileHeapVar(VarSymbol& _var)
	{
		//reference uses rawPtr, no extra space is required
		//todo: make above true
	//	if(!_var.typeInfo.isReference)
		_var.ownership.rawPtr = m_runtime.getMemMgr()->alloc(_var.typeInfo.type.size);
		_var.ownership.ownerType = OwnershipType::Module;
		_var.isPtr = true; 
		_var.typeInfo.isReference = true;  // enforce reference assignment
	}

	// *************************************************** //

	void Compiler::compileModuleInit(NaReTi::Module& _module)
	{
		if (_module.m_text->size() == 0) return;
		
		m_compiler.addFunc(FuncSignature0<void>());
		resetRegisters();
		compileCode(*_module.m_text);

		m_compiler.endFunc();
		m_compiler.finalize();
		void* ptr;
		Error err = m_runtime.add(&ptr, &m_codeHolder);
		((NaReTi::basicFunc*)ptr)();
		m_codeHolder.reset(true);
		m_codeHolder.init(m_runtime.getCodeInfo());
		m_codeHolder.attach(&m_compiler);
		m_runtime.release(ptr); // this function wont be needed anymore
	}

	// *************************************************** //

	void Compiler::convertSignature(par::Function& _function) const
	{
		FuncSignatureX& FuncSignature = _function.funcBuilder;
		for (int i = 0; i < _function.paramCount; ++i)
		{
			switch (_function.scope.m_variables[i]->typeInfo.type.basic)
			{
			case BasicType::Int: FuncSignature.addArgT<int>(); break;
			case BasicType::Float: FuncSignature.addArgT<float>(); break;
			case BasicType::Complex: FuncSignature.addArgT<void*>(); break;
			default:
				if (_function.scope.m_variables[i]->typeInfo.isReference) FuncSignature.addArgT<void*>();
				else FuncSignature.addArgT<int>();
			}
		}
		switch (_function.returnTypeInfo.type.basic)
		{
		case BasicType::Int: FuncSignature.setRetT<int>(); break;
		case BasicType::Float: FuncSignature.setRetT<float>(); break;
		case BasicType::Complex: FuncSignature.setRetT<void*>(); break;
		default: 
			if (_function.returnTypeInfo.isReference) FuncSignature.setRetT<void*>();
			else FuncSignature.setRetT<void>();
		}

	}

	// *************************************************** //

	void Compiler::allocVar(VarSymbol& _sym)
	{
		asmjit::Operand* varPtr;

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

	X86Gp* Compiler::allocStackVar(const ComplexType& _type, int _count)
	{
		X86Gp& gpVar = getUnusedVar();
		X86Mem mem = m_compiler.newStack(_type.size * _count, _type.alignment); // if floats are used  -> operations with xmm(128) register

		m_compiler.lea(gpVar, mem);

		return &gpVar;
	}

	// *************************************************** //

	void Compiler::compileFuction(par::Function& _function)
	{
		//intrinsics are always inlined
		if (_function.bIntrinsic) return;

		m_function = &_function;

		//setup signature
		convertSignature(_function);
		//externals are not compiled
		//but still require a signature to be called
		if (_function.bExternal) return;
		m_compiler.addFunc(_function.funcBuilder);


		//setup registers
		resetRegisters();

		if (_function.name == "rand")
			int brk = 0;

		//code
		UsageStateLock lock(m_usageState);

		compileCode(_function.scope);
		
		for (int i = 0; i < _function.paramCount; ++i)
			m_compiler.setArg(i, *static_cast<Reg*>(_function.scope.m_variables[i]->compiledVar));

		// Finalize the current function.
		m_compiler.endFunc();
		m_compiler.finalize();
		void* ptr;
		Error err = m_runtime.add(&ptr, &m_codeHolder);
		m_codeHolder.reset(true);
		m_codeHolder.init(m_runtime.getCodeInfo());
		m_codeHolder.attach(&m_compiler);
	//	m_compiler._code = &m_codeHolder;
		_function.binary = ptr;
	}

	// *************************************************** //
	void Compiler::compileCode(ASTCode& _node, int _preAllocCount)
	{
		//create local vars in this scope
		for (int i = _preAllocCount; i < _node.m_variables.size(); ++i)
		{
			VarSymbol& varSymbol = *_node.m_variables[i];
			if (varSymbol.isSubstituted) continue;

			allocVar(varSymbol);
		}

		UsageStateLock lock(m_usageState);

		//imported heap vars
		for (auto& var : _node.m_importedVars)
		{
			X86Gp& gpVar = getUnusedVar(); //is always a ptr
			var.sym->compiledVar = &gpVar;
			m_compiler.mov(gpVar, asmjit::imm_ptr(var.sym->ownership.rawPtr));

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
				compileCall(*(ASTCall*)subNode, false);
				break;
			case ASTType::Ret:
				// return needs the current scope to call its epilogue
				m_currentCodeNode = &_node;
				ASTReturn* retNode = (ASTReturn*)subNode;
				if (retNode->body && retNode->body->typeInfo->type.basic == BasicType::Float && !retNode->body->typeInfo->isReference)
					compileRetF(*(ASTReturn*)subNode);
				else compileRet(*(ASTReturn*)subNode);
				break;
			}
		}

		compileEpilogue(_node);
	}

	// *************************************************** //
	void Compiler::compileEpilogue(const par::ASTCode& _node)
	{
		//write back changed addresses of globals
		//todo: rework way globals are handled
		//they should not be imported to regs/stack, instead use mem access
		for (auto& var : _node.m_importedVars)
		{
			if (var.refHasChanged)
			{
				X86Gp& gpVar = getUnusedVar();
				m_compiler.mov(gpVar, asmjit::imm_ptr(&var.sym->ownership.rawPtr));

				//	m_compiler.mov(*(X86Gp*)var.sym->compiledVar, imm(2));
				m_compiler.mov(asmjit::X86Mem(gpVar, 0), *(X86Gp*)var.sym->compiledVar);
			}
		}
		for (auto& subNode : _node.epilogue)
		{
			switch (subNode->type)
			{
			case ASTType::Call:
				compileCall(*(ASTCall*)subNode, false);
				break;
			case ASTType::Branch:
			case ASTType::Loop:
			case ASTType::Code:
			case ASTType::Ret:
				assert(true && "There should be no such statement in the epilogue");
				break;
			}
		}
		// resolve higher level scopes; stop before reaching the global scope
		if (_node.parent && _node.parent->parent) compileEpilogue(*_node.parent);
	}

	// *************************************************** //

	Operand* Compiler::compileCall(ASTCall& _node, bool _keepRet, asmjit::Operand* _dest)
	{
		Function& func = *_node.function;
		if (func.name == "DisplayValue")
			int brk = 0;

		std::vector< asmjit::Operand* > args; args.reserve(_node.args.size());

		//return values are part of the caller's scope
		if (func.bHiddenParam)
		{
			if (_node.returnSub) // optimization
			{
				args.push_back(_node.returnSub->compiledVar);
			}
			else
			{
				//allocate the stack var and provide a reference as param
				func.scope.m_variables[0]->compiledVar = (Operand*)allocStackVar(func.scope.m_variables[0]->typeInfo.type);
				args.push_back(func.scope.m_variables[0]->compiledVar);
			}
			_dest = args.back();
		}

		UsageStateLock lock(m_usageState);

		if (!_dest) _dest = getUnusedVarAuto(func.returnTypeInfo);

		if (_keepRet) lock.reset();

		int indirect = 0;
		int i = 0; //identify the first operand
		//make sure that all arguments are located in virtual registers
		for (auto& arg : _node.args)
		{
		//	auto& arg = *begin;
			switch (arg->type)
			{
			case ASTType::LeafInt:
			case ASTType::LeafFloat:
			case ASTType::LeafSym:
			case ASTType::Leaf:
			case ASTType::LeafSizeOf:
				ASTLeaf* leaf; leaf = (ASTLeaf*)arg;
				args.push_back(compileLeaf(*leaf, &indirect));
				break;
			case ASTType::LeafAddress:
			{
				//address constants are always typecasted to the required type
				//this cast is free and combined with the necessary load
				ASTLeafAdr* adr = static_cast<ASTLeafAdr*>(arg);
				X86Gp& var = getUnusedVar();
				m_compiler.mov(var, Imm(adr->value));
				args.push_back(&var);
				break;
			}
			case ASTType::String:
				assert(false && "This type is not in use anymore.");
				break;
			case ASTType::LeafString:
			{
				X86Gp& var = getUnusedVar();
				args.push_back(&var);
				m_compiler.mov(var, imm_ptr(&(static_cast<ASTLeafStr*>(arg)->value))); //->value.buf
				break;
			}
			case ASTType::Member:
			{
				ASTMember& member = *(ASTMember*)arg;
				X86Gp* baseVar = compileMemberAdr(member);
				
				if (func.intrinsicType == Function::Assignment && i == 0)
				{
					X86Gp* var = &getUnusedVar();
					args.push_back(var);
					m_compiler.lea(*var, getMemberAdr(*(ASTMember*)arg, *baseVar));
					indirect += 0x100;

					// member reference with actual assignment
					// todo: make global decision on indirection
					if (func.name == "=" && arg->typeInfo->isReference)
					{
						auto addr = x86::ptr(*var);
						addr.setSize(PTRSIZE);
						m_compiler.mov(*var, addr);
					}
				}
				else
				{
					Reg* var = getUnusedVarAuto(*member.typeInfo);
					args.push_back(var);
					if (var->getKind() == 1) // xmm var
						compileMemberLdF(*(ASTMember*)arg, *baseVar, *(X86Xmm*)var);
					else
						compileMemberLd(*(ASTMember*)arg, *baseVar, *(X86Gp*)var);
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
			i++;
		}

		if (_node.function->bInline)
		{
			if (_node.function->bIntrinsic)
			{
				switch (_node.function->intrinsicType)
				{
				case Function::Assignment:
					_dest = args[0]; // assignments return their left site operand
					break;
				case Function::BinOp:
					//since the first operand of a binop is overwritten with the result copy the values first
					//todo movOpt
					if (static_cast<Reg*>(_dest)->getKind() == 1)
					{
						m_compiler.movss(*(X86Xmm*)_dest, *(X86Xmm*)args[0]);
					}
					else
					{
						m_compiler.mov(*(X86Gp*)_dest, *(X86Gp*)args[0]);
					}
					args[0] = _dest;
					break;
				case Function::TypeCast:
					args.push_back(_dest);
					break;
				case Function::Compare: break;
				default: break;
				case Function::StaticCast:
					assert(false && "static casts should be skipped in the parser.");
					break;
				}

				for (auto& node : _node.function->scope)
				{
					ASTOp& op = *(ASTOp*)node;
					compileOp(op.instruction, args, indirect);
				}
			}
			else // general purpose inline
			{
				for (int i = 0; i < args.size(); ++i)
					func.scope.m_variables[i]->compiledVar = args[i];
				
				//returns -> jump to the end of this block
				m_retDstStack.emplace_back(_dest, m_compiler.newLabel());
				Function* callee = m_function;
				m_function = _node.function;
				compileCode(_node.function->scope, args.size());
				m_function = callee;
				m_compiler.bind(m_retDstStack.back().second);
				m_retDstStack.pop_back();
			}
		} // end if inline
		else
		{
			// make call
			CCFuncCall* call = m_compiler.call(imm_ptr(func.binary), func.funcBuilder);
			for (int i = 0; i < func.paramCount; ++i)
				call->_setArg(i, *args[i]);

			if(_dest) call->setRet(0, *static_cast<Reg*>(_dest));
		}

		if (func.name == "breakpoint")
			int uidae = 2l;

		return _dest;
	}

	// *************************************************** //

	asmjit::Operand* Compiler::compileLeaf(par::ASTLeaf& _node, int* _indirect)
	{
		//immediate val
		if (_node.type == ASTType::LeafInt)
		{
			X86Gp& var = getUnusedVar32();
			m_compiler.mov(var, Imm(((ASTLeafInt*)&_node)->value));
			return &var;
		}
		else if (_node.type == ASTType::LeafFloat)
		{
			X86Xmm& var = getUnusedFloat();
			m_compiler.movss(var, m_compiler.newFloatConst(0, ((ASTLeafFloat*)&_node)->value));
			return &var;
		}
		else if (_node.type == ASTType::LeafSym)
		{
			//problem:
			// there are 2 levels of indirection-detection:
			// 1. member of instance is always indirect access, any recursive levels are resolved
			// 2. pointer var, requires indirect access only when it's content should be changed
			if (_indirect && ((ASTLeafSym*)&_node)->value->isPtr) (*_indirect)++;
			return ((ASTLeafSym*)&_node)->value->compiledVar;
		}
		else if (_node.type == ASTType::LeafSizeOf)
		{
			X86Gp& var = getUnusedVar32();
			ASTSizeOf& node = *(ASTSizeOf*)&_node;

			m_compiler.mov(var, Imm(node.value.isReference ? PTRSIZE : node.value.type.size));
			return &var;
		}
		assert(false && "Could not compile leaf."); // could not compile leaf
		return nullptr;
	}

	// *************************************************** //

	void Compiler::compileOp(par::InstructionType _instr, const std::vector< asmjit::Operand* >& _args, int _indirect)
	{
		switch (_instr)
		{
		case Inc:
			m_compiler.inc(*(X86Gp*)_args[0]);
			break;
		case Dec:
			m_compiler.dec(*(X86Gp*)_args[0]);
			break;
		case Neg:
			m_compiler.mov(*(X86Gp*)_args[1], *(X86Gp*)_args[0]); //val is copied to not change the original and to have the result in the right destination
			m_compiler.neg(*(X86Gp*)_args[1]);
			break;
		case Add:
			m_compiler.add(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Sub:
			m_compiler.sub(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Mul:
			m_compiler.imul(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Mod:
		{
			X86Gp& dummy = getUnusedVar32();
			m_compiler.xor_(dummy, dummy);
			m_compiler.idiv(dummy, *(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			m_compiler.mov(*(X86Gp*)_args[0], dummy);
			break;
		}
		case Div:
		{
			X86Gp& dummy = getUnusedVar();
			m_compiler.xor_(dummy, dummy);
			m_compiler.idiv(dummy, *(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		}
		case ShL:
			m_compiler.shl(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case ShR:
			m_compiler.shr(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case And:
			m_compiler.and_(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Xor:
			m_compiler.xor_(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Or:
			m_compiler.or_(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Mov:
			if (_indirect >= 0x100){
				auto addr = x86::ptr(*(X86Gp*)_args[0]);
				addr.setSize(PTRSIZE);
				m_compiler.mov(addr, *(X86Gp*)_args[1]);
			}
			else m_compiler.mov(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case Set:
			if (_indirect) {
				m_compiler.mov(x86::dword_ptr(*(X86Gp*)_args[0]), *(X86Gp*)_args[1]);
			} else
				m_compiler.mov(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case fSet:
			if (_indirect) {
				m_compiler.movss(x86::dword_ptr(*(X86Gp*)_args[0]), *(X86Xmm*)_args[1]);
			}
			else
				m_compiler.movss(*(X86Xmm*)_args[0], *(X86Xmm*)_args[1]);
			break;
		case Ld:
			m_compiler.mov(*(X86Gp*)_args[1], x86::dword_ptr(*(X86Gp*)_args[0]));
			break;
		case fLd:
			m_compiler.movss(*(X86Xmm*)_args[1], x86::dword_ptr(*(X86Gp*)_args[0]));
			break;
		case LdO:
			m_compiler.lea(*(X86Gp*)_args[2], x86::dword_ptr(*(X86Gp*)_args[0], *(X86Gp*)_args[1], 0)); //2
			break;
		case Cmp:
			m_compiler.cmp(*(X86Gp*)_args[0], *(X86Gp*)_args[1]);
			break;
		case CmpZ:
			m_compiler.cmp(*(X86Gp*)_args[0], imm(0));
			break;
		case fCmp:
			m_compiler.comiss(*(X86Xmm*)_args[0], *(X86Xmm*)_args[1]);
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
		case fAdd:
			m_compiler.addss(*(X86Xmm*)_args[0], *(X86Xmm*)_args[1]);
			break;
		case fSub:
			m_compiler.subss(*(X86Xmm*)_args[0], *(X86Xmm*)_args[1]);
			break;
		case fMul:
			m_compiler.mulss(*(X86Xmm*)_args[0], *(X86Xmm*)_args[1]);
			break;
		case fDiv:
			m_compiler.divss(*(X86Xmm*)_args[0], *(X86Xmm*)_args[1]);
			break;
		case fNeg:{
			auto zeroConst = m_compiler.newFloatConst(0, 0.f); //todo: allocate this cost just once
			m_compiler.movss(*(X86Xmm*)_args[1], zeroConst); 
			m_compiler.subss(*(X86Xmm*)_args[1], *(X86Xmm*)_args[0]);
			break; }
		case Sqrt:
			m_compiler.sqrtss(*(X86Xmm*)_args[1], *(X86Xmm*)_args[0]);
			break;
		//typecasts
		case iTof:
			m_compiler.cvtsi2ss(*(X86Xmm*)_args[1], *(X86Gp*)_args[0]);
			break;
		case fToi:
			m_compiler.cvtss2si(*(X86Gp*)_args[1], *(X86Xmm*)_args[0]);
			break;
		}
	}

	// *************************************************** //

	void Compiler::compileRet(const ASTReturn& _node)
	{
		UsageStateLock lock(m_usageState);

		//void f()
		if (!_node.body)
		{
			m_compiler.ret();
			return;
		}

		asmjit::X86Gp* var = nullptr;
		if (_node.body->type == ASTType::Call)
		{
			var = (X86Gp*)compileCall(*(ASTCall*)_node.body);
		}
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			var = &getUnusedVar();
			compileMemberLd(member, *compileMemberAdr(member), *var);
		}
		else if ((int)_node.body->type >= (int)ASTType::Leaf)
		{
			var = (X86Gp*)compileLeaf(*(ASTLeaf*)_node.body);
		}

		if (m_function->bHiddenParam)
		{
			X86Gp& dest = *(X86Gp*)m_function->scope.m_variables[0]->compiledVar;
			//due to substitution the value can already be in the destination
		//	if(&dest != var) compileMemCpy(dest, *var, m_function->returnTypeInfo.type.size);

			var = &dest;
		}

		compileEpilogue(*m_currentCodeNode);

		//currently inlining
		if (m_retDstStack.size())
		{
			if (var != m_retDstStack.back().first) m_compiler.mov(*(X86Gp*)m_retDstStack.back().first, *var);
			m_compiler.jmp(m_retDstStack.back().second);
		}
		else
			m_compiler.ret(*var);
	}

	// *************************************************** //

	void Compiler::compileRetF(const ASTReturn& _node)
	{
		UsageStateLock lock(m_usageState);

		X86Xmm* var;
		if (_node.body->type == ASTType::Call)
		{
			var = (X86Xmm*)compileCall(*(ASTCall*)_node.body);
		}
		else if (_node.body->type == ASTType::Member)
		{
			ASTMember& member = *(ASTMember*)_node.body;
			var = &getUnusedFloat();
			compileMemberLdF(member, *compileMemberAdr(member), *var);
		}
		else if ((int)_node.body->type >= (int)ASTType::Leaf)
		{
			var = (X86Xmm*)compileLeaf(*(ASTLeaf*)_node.body);
		}

		compileEpilogue(*m_currentCodeNode);

		if (m_retDstStack.size())
		{
			if (var != m_retDstStack.back().first) m_compiler.movss(*(X86Xmm*)m_retDstStack.back().first, *var);
			m_compiler.jmp(m_retDstStack.back().second);
		}
		else
			m_compiler.ret(*var);
	}

	// *************************************************** //

	void Compiler::compileMemCpy(const X86Gp& _dst, const X86Gp& _src, size_t _size)
	{
		X86Gp& tmp = getUnusedVar();
		size_t chunks = _size / 4;
		for (size_t i = 0; i < chunks; ++i)
		{
			m_compiler.mov(tmp, x86::dword_ptr(_src, i*4));
			m_compiler.mov(x86::dword_ptr(_dst, i*4), tmp);
		}
	}

	// *************************************************** //

	X86Gp* Compiler::compileMemberAdr(par::ASTMember& _node)
	{
		X86Gp* baseVar;
		switch (_node.instance->type)
		{
		case ASTType::Call:
			baseVar = (X86Gp*)compileCall(*(ASTCall*)_node.instance); // take a var that has a higher lifespan that is still valid in getMemberAdr
			break;
		case ASTType::LeafSym:
		{
			ASTLeafSym& leaf = *(ASTLeafSym*)_node.instance;
			baseVar = (X86Gp*)leaf.value->compiledVar;
			break;
		}
		case ASTType::Member:
		{
			ASTMember& inst = *(ASTMember*)_node.instance;
			//first go down to the instance that is actually loaded in this scope
			//problem: when the member is no reference compileMemberLd will still load a 
			// ptr at that location instead of just adding to the offset
			baseVar = compileMemberAdr(inst);
		
			// the allocated var containing the instance is set free
			// and will be reused throughout the recursion.
			// since nothing can happen between the end of this scope and the
			// assignment above the var can not be overwritten
			UsageStateLock lock(m_usageState);

			auto& var = getUnusedVar();
			if (inst.instance->typeInfo->type.scope.m_variables[inst.index]->typeInfo.isReference)
				compileMemberLd(inst, *baseVar, var);
			else //todo: make offset addition in compile time
			{
				m_compiler.lea(var, getMemberAdr(inst, *baseVar));
			}
			baseVar = &var;

			break;
		}
		}

		assert(baseVar);
		return baseVar;
	}

	// *************************************************** //

	X86Mem Compiler::getMemberAdr(ASTMember& _node, X86Gp& _var)
	{
		ComplexType& type = _node.instance->typeInfo->type;
		return x86::dword_ptr(_var, type.displacement[_node.index]);
	}

	// *************************************************** //

	void Compiler::compileMemberLd(ASTMember& _node, X86Gp& _var, X86Gp& _destination)
	{
		auto adr = getMemberAdr(_node, _var);

		//adjust target size
		adr.setSize(_destination.getSize());
		m_compiler.mov(_destination, adr);
	}

	// *************************************************** //

	void Compiler::compileMemberLdF(ASTMember& _node, X86Gp& _var, X86Xmm& _destination)
	{
		auto adr = getMemberAdr(_node, _var);
		
		m_compiler.movss(_destination, adr);
	}

	// *************************************************** //

	void Compiler::compileBranch(ASTBranch& _node)
	{
		Label end(m_compiler.newLabel());
		m_labelStack.emplace_back(m_compiler.newLabel());
		Label& elseBranch = m_labelStack.back();
		
		compileCondExp(*(ASTCall*)_node.condition);
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
		Label begin(m_compiler.newLabel());

		m_labelStack.emplace_back(m_compiler.newLabel());
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
			m_labelStack.emplace_back(m_compiler.newLabel());
			Label& end = m_labelStack.back();
			Label postOr(m_compiler.newLabel());

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
		else compileCall(_node, false);
	}

	// *************************************************** //

	asmjit::X86Gp& Compiler::getUnusedVar()
	{
		if (m_anonymousVars.size() == m_usageState.varsInUse) m_anonymousVars.push_back(m_compiler.newIntPtr());

		return m_anonymousVars[m_usageState.varsInUse++];
	}

	// *************************************************** //

	asmjit::X86Gp& Compiler::getUnusedVar32()
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

	asmjit::X86Xmm& Compiler::getUnusedFloat()
	{
		if (m_anonymousFloats.size() == m_usageState.floatsInUse) m_anonymousFloats.push_back(m_compiler.newXmmSs());

		return m_anonymousFloats[m_usageState.floatsInUse++];
	}

	// *************************************************** //

	asmjit::Reg* Compiler::getUnusedVarAuto(const TypeInfo& _typeInfo)
	{
		if (_typeInfo.isReference || _typeInfo.type.basic == BasicType::Complex) //complex is always a reference
			return &getUnusedVar();
		else if (_typeInfo.type.basic == BasicType::Float)
			return &getUnusedFloat();
		else if (_typeInfo.type.size <= 4)
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