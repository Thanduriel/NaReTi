#include "optimizer.hpp"
#include <assert.h>

namespace codeGen{
	using namespace par;

	//amount of calls a function may contain to be inlined
	const int InlineTreshhold = 6;

	Optimizer::Optimizer()
	{
		m_tempPtrs.reserve(32);
	}

	void Optimizer::optimize(NaReTi::Module& _module)
	{
		m_module = &_module;
		for (auto& func : _module.m_functions)
			optimizeFunction(*func);
	}

	void Optimizer::optimizeFunction(par::Function& _func)
	{
		//externals can not be optimized
		if (_func.bExternal) return;

		for (int i = 0; i < _func.paramCount; ++i)
		{
			//assume that every var is const
			_func.scope.m_variables[i]->typeInfo.isConst = true;
		}

		m_function = &_func;
		m_callCount = 0;
		m_usageStack.clear();
		m_tempPtrs.clear();
		traceCode(_func.scope);

		bool isConst = true;
		for (int i = 0; i < _func.paramCount; ++i)
		{
			if (!_func.scope.m_variables[i]->typeInfo.isConst)
			{
				isConst = false;
				break;
			}
		}
		if (m_callCount <= InlineTreshhold && isConst)
			_func.bInline = true;
	}

	// ****************************************************** //

	void Optimizer::traceNode(ASTNode* _node, ASTExpNode** _dest)
	{
		switch (_node->type)
		{
		case ASTType::Code:
			traceCode(*(ASTCode*)_node);
			break;
		case ASTType::Branch:
			traceBranch(*(ASTBranch*)_node);
			break;
		case ASTType::Loop:
			traceLoop(*(ASTLoop*)_node);
			break;
		case ASTType::Call:
			traceCall(*(ASTCall*)_node, _dest);
			break;
		case ASTType::Ret:
			traceReturn(*(ASTReturn*)_node);
			break;
		case ASTType::Member:
			traceMember(*(ASTMember*)_node);
			break;
		case ASTType::LeafSym:
			traceLeaf(*(ASTLeafSym*)_node);
			break;
		}
	}

	void Optimizer::traceCode(ASTCode& _node)
	{
		for (auto& subNode : _node)
		{
			traceNode(subNode, (ASTExpNode**)&subNode);
		}
	}

	void Optimizer::traceBranch(ASTBranch& _node)
	{
		traceCall(*(ASTCall*)_node.condition, &_node.condition);
		traceCode(*_node.ifBody);
		if (_node.elseBody) traceCode(*_node.elseBody);
	}

	void Optimizer::traceLoop(ASTLoop& _node)
	{
		traceCall(*(ASTCall*)_node.condition, &_node.condition);
		traceNode(_node.body);
	}

	// ****************************************************** //

	void Optimizer::traceCall(ASTCall& _node, ASTExpNode** _dest)
	{
		m_callCount++;

		int stackLabel = (int)m_usageStack.size();

		//trace arg nodes
		for (int i = 0; i < (int)_node.args.size(); ++i)
		{
			auto& arg = _node.args[i];
			traceNode(arg, &arg);
			// discard const qualifier when a call requires a non const arg
			if (arg->type == ASTType::LeafSym && !_node.function->scope.m_variables[i]->typeInfo.isConst)
				((ASTLeafSym*)arg)->value->typeInfo.isConst = false;
		}
		tryConstFold(_node, _dest);

		// expressions of the form: a = foo()
		if (_node.function->name == "=" && _node.args[1]->type == ASTType::Call && _node.args[0]->type == ASTType::LeafSym)
		{
			VarSymbol* sym = ((ASTLeafSym*)_node.args[0])->value;

			stackLabel++; //left site operand of "=" is of course the var itself
			//if the var is given as param it may still be used in it's original form
			for (; stackLabel < m_usageStack.size(); stackLabel++)
				if (*m_usageStack[stackLabel] == sym) return;

			ASTCall* rCall = (ASTCall*)_node.args[1];
			if (rCall->function->bHiddenParam)
			{
				//the assignment is replaced with the right site call 
				// and the var destination is given as return sub
				_node.returnSub = ((ASTLeafSym*)_node.args[0])->value;
				_node.function = rCall->function;
				_node.name = rCall->name;
				_node.args = rCall->args;
				_node.typeInfo = rCall->typeInfo;
			}
		}
	}

	// ****************************************************** //

	void Optimizer::traceReturn(ASTReturn& _node)
	{
		traceNode(_node.body, &_node.body);
		
		if (m_function->bHiddenParam)
		{
			trySubstitution(**m_usageStack.back(), *m_function->scope.m_variables[0]);

			//put return var on the stack as it can not be used again
			m_tempPtrs.push_back(m_function->scope.m_variables[0]);
			m_usageStack.push_back(&m_tempPtrs.back());
		}
	}

	void Optimizer::traceMember(ASTMember& _node)
	{
		traceNode(_node.instance);
	}

	void Optimizer::traceLeaf(ASTLeafSym& _node)
	{
		m_usageStack.push_back(&_node.value);
	}

	// ****************************************************** //

	void Optimizer::tryConstFold(ASTCall& _node, ASTExpNode** _dest)
	{
		assert(_dest != nullptr);

		if (!_node.function->bIntrinsic) return;
		for (auto& arg : _node.args) if (arg->type != ASTType::LeafInt && arg->type != ASTType::LeafFloat) return;

		for (auto node : _node.function->scope)
		{
			ASTOp& op = *(ASTOp*)node;
			switch (op.instruction)
			{
			case Add:
				((ASTLeafInt*)_node.args[0])->value += ((ASTLeafInt*)_node.args[1])->value;
				*_dest = _node.args[0];
				break;
			case Sub:
				((ASTLeafInt*)_node.args[0])->value -= ((ASTLeafInt*)_node.args[1])->value;
				*_dest = _node.args[0];
				break;
			case iTof:
				*_dest = m_module->getAllocator().construct < ASTLeafFloat >((float)((ASTLeafInt*)_node.args[0])->value);
				break;
			case fToi:
				*_dest = m_module->getAllocator().construct < ASTLeafInt >((int)((ASTLeafFloat*)_node.args[0])->value);
				break;
			default:
				return; // non implemented instructions -> not folded
			}
			(*_dest)->typeInfo = _node.typeInfo;
		}
	}

	// ****************************************************** //

	void Optimizer::trySubstitution(VarSymbol& _target, VarSymbol& _sub)
	{
		//verify that the sub is not used before
		for (auto& var : m_usageStack)
		{
			//is already in use
			if (*var == &_sub) return;
		}

		_target.isSubstituted = true;
		//actual substitution
		for (auto& var : m_usageStack)
		{
			if (*var == &_target) *var = &_sub;
		}
	}
}