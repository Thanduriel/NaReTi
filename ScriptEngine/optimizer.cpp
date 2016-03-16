#include "optimizer.hpp"

namespace codeGen{
	using namespace par;

	const int InlineTreshhold = 5;

	Optimizer::Optimizer()
	{
		m_tempPtrs.reserve(32);
	}

	void Optimizer::optimize(NaReTi::Module& _module)
	{
		for (auto& func : _module.m_functions)
			optimizeFunction(*func);
	}

	void Optimizer::optimizeFunction(par::Function& _func)
	{
//		if (_func.scope.size() < InlineTreshhold) _func.bInline = true;
		
		m_function = &_func;
		m_usageStack.clear();
		m_tempPtrs.clear();
		traceCode(_func.scope);
	}

	// ****************************************************** //

	void Optimizer::traceNode(ASTNode* _node)
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
			traceCall(*(ASTCall*)_node);
			break;
		case ASTType::Ret:
			traceReturn(*(ASTReturn*)_node);
			break;
		case ASTType::Member:
			traceMember(*(ASTMember*)_node);
			break;
		case ASTType::Leaf:
			traceLeaf(*(ASTLeaf*)_node);
			break;
		}
	}

	void Optimizer::traceCode(ASTCode& _node)
	{
		for (auto& subNode : _node)
		{
			traceNode(subNode);
		}
	}

	void Optimizer::traceBranch(ASTBranch& _node)
	{
		traceCall(*(ASTCall*)_node.condition);
		traceCode(*_node.ifBody);
		if (_node.elseBody) traceCode(*_node.elseBody);
	}

	void Optimizer::traceLoop(ASTLoop& _node)
	{
		traceCall(*(ASTCall*)_node.condition);
		traceNode(_node.body);
	}

	void Optimizer::traceCall(ASTCall& _node)
	{
		for (auto& arg : _node.args)
		{
			traceNode(arg);
		}
		// a = foo()
		if (_node.function->name == "=" && _node.args[1]->type == ASTType::Call)
		{
			ASTCall* rCall = (ASTCall*)_node.args[1];
			if (rCall->function->bHiddenParam)
			{
				rCall->returnSub = ((ASTLeaf*)_node.args[0])->ptr;
			}
		}
	}

	void Optimizer::traceReturn(ASTReturn& _node)
	{
		traceNode(_node.body);
		
		if (m_function->bHiddenParam)
		{
			trySubstitution(**m_usageStack.back(), m_function->scope.m_variables[0]);

			//put return var on the stack as it can not be used again
			m_tempPtrs.push_back(&m_function->scope.m_variables[0]);
			m_usageStack.push_back(&m_tempPtrs.back());
		}
	}

	void Optimizer::traceMember(ASTMember& _node)
	{
		traceNode(_node.instance);
	}

	void Optimizer::traceLeaf(ASTLeaf& _node)
	{
		if (_node.parType == ParamType::Ptr)
			m_usageStack.push_back(&_node.ptr);
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