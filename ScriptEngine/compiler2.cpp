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

		m_anonymousVars.clear();
		m_anonymousVars.reserve(32); // make shure that no move will occure
		
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
			case ASTType::BinOp:
				compileBinOp(*(ASTBinOp*)subNode);
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

	void Compiler::compileCall(ASTCall& _node, size_t _anonUsed)
	{
		std::vector< X86GpVar* > args; args.resize(_node.args.size());
		size_t argPtr = _anonUsed;
		if (_node.function->bInline)
		{
			int i = 0;
			//args
			for (auto& arg : _node.args)
			{
				switch (arg->type)
				{
				case ASTType::Leaf:
					ASTLeaf* leaf; leaf = (ASTLeaf*)arg;
					//immediate val
					if (leaf->parType == ParamType::Int)
					{
						if (m_anonymousVars.size() == argPtr) m_anonymousVars.emplace_back(m_compiler.newInt32());
						args[i] = &m_anonymousVars[argPtr];
						m_compiler.mov(m_anonymousVars[argPtr], Imm(leaf->val));
						argPtr++;
					}
					else
					{
						// copy arg[0] to not change its value
						if (i == 0)
						{
							m_compiler.mov(m_accumulator, ((ASTLeaf*)arg)->ptr->binVar);
							args[i] = &m_accumulator;
						}
						else args[i] = &((ASTLeaf*)arg)->ptr->binVar;
					}
					break;
				case ASTType::Call: 
					if (m_anonymousVars.size() == argPtr) m_anonymousVars.emplace_back(m_compiler.newInt32());
					args[i] = &m_anonymousVars[argPtr];
					compileCall(*(ASTCall*)arg, argPtr);
					m_compiler.mov(m_anonymousVars[argPtr], m_accumulator);
					argPtr++;
					break;
				}
				i++;
			}
			//code
			ASTBinOp& binOp = *(ASTBinOp*)_node.function->scope[0];

			switch (binOp.instruction)
			{
			case InstructionType::Add:
				m_compiler.add(*args[0], *args[1]);
				break;
			case InstructionType::Sub:
				m_compiler.sub(*args[0], *args[1]);
				break;
			case InstructionType::Mul:
				m_compiler.imul(*args[0], *args[1]);
				break;
			}
			m_compiler.mov(m_accumulator, *args[0]);
		}
	}

	// *************************************************** //

	void Compiler::compileBinOp(ASTBinOp& _node)
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
	}

	// *************************************************** //

	void Compiler::compileRet(ASTReturn& _node)
	{
		if (_node.body->type == ASTType::Call) compileCall(*(ASTCall*)_node.body);
		else if (_node.body->type == ASTType::BinOp) compileBinOp(*(ASTBinOp*)_node.body);

		m_compiler.ret(m_accumulator);
	}
}