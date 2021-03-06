#include "optimizer.hpp"
#include "module.hpp"
#include "ast.hpp"
#include <assert.h>
#include <algorithm>

#define FOLDCONST(type, op) ((type*)_node.args[0])->value op ((type*)_node.args[1])->value; *_dest = _node.args[0]; break;

//#define OPTIMIZEINIT

namespace codeGen{
	using namespace par;

	//amount of calls a function may contain to be inlined
	constexpr int InlineTreshhold = 6;

	Optimizer::Optimizer()
	{
		m_tempPtrs.reserve(32);
	}

	void Optimizer::optimize(NaReTi::Module& _module)
	{
		m_module = &_module;

#ifdef OPTIMIZEINIT
		m_function = nullptr;
		resetState();
		//since this happens before compilation ownership information is not correct
		for (auto& var : _module.m_text->m_variables)
			var->ownership = OwnershipType::Module;
		traceCode(*_module.m_text);
#endif

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
		resetState();
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

	void Optimizer::resetState()
	{
		m_callCount = 0;
		m_usageStack.clear();
		m_tempPtrs.clear();
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
		m_scope = &_node;

		for (auto& var : _node.m_importedVars)
		{
			var.refHasChanged = false; //assume no changes and set if necessary in traceCall
		}

		for (auto& subNode : _node)
		{
			traceNode(subNode, (ASTExpNode**)&subNode);
		}

		m_scope = _node.parent;
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
		//ref count and global import/export
		else if (_node.function->name == ":=")
		{
			if (_node.args[0]->type == ASTType::LeafSym)
			{
				ASTLeafSym* leaf = static_cast<ASTLeafSym*>(_node.args[0]);
				if (leaf->value->ownership.ownerType == OwnershipType::Heap 
					|| leaf->value->ownership.ownerType == OwnershipType::Module)
				{
					auto it = std::find_if(m_scope->m_importedVars.begin(), m_scope->m_importedVars.end(),
						[&](const CodeScope::ImportedVar& _var){return _var.sym == leaf->value; });
					it->refHasChanged = true;
				}
			}
		}
	}

	// ****************************************************** //

	void Optimizer::traceReturn(ASTReturn& _node)
	{
		//void f()
		if (!_node.body) return;

		traceNode(_node.body, &_node.body);
		
		//only when a local var is returned substitution may take place
		if (m_function && m_function->bHiddenParam && _node.body->type == ASTType::LeafSym)
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

	void Optimizer::traceLeaf(ASTExpNode& _node)
	{
		m_usageStack.push_back(&((ASTLeafSym*)&_node)->value);
	}

	// ****************************************************** //

	void Optimizer::tryConstFold(ASTCall& _node, ASTExpNode** _dest)
	{
		//requires a destination to place the new node in
		if (!_dest) return;
		//assert(_dest != nullptr);

		if (!_node.function->bIntrinsic) return;
		for (auto& arg : _node.args) if (arg->type != ASTType::LeafInt && arg->type != ASTType::LeafFloat) return;

		for (auto node : _node.function->scope)
		{
			ASTOp& op = *(ASTOp*)node;
			switch (op.instruction)
			{
			case Add:	FOLDCONST(ASTLeafInt, += );
			case Sub:	FOLDCONST(ASTLeafInt, -= );
			case Mul:	FOLDCONST(ASTLeafInt, *= );
			case Div:	FOLDCONST(ASTLeafInt, /= );
			case Mod:	FOLDCONST(ASTLeafInt, %= );
			case ShL:	FOLDCONST(ASTLeafInt, <<= );
			case ShR:	FOLDCONST(ASTLeafInt, >>= );
			case Xor:	FOLDCONST(ASTLeafInt, ^= );
			case Or:	FOLDCONST(ASTLeafInt, |= );
			case And:	FOLDCONST(ASTLeafInt, &= );
			case fAdd:	FOLDCONST(ASTLeafFloat, += );
			case fSub:	FOLDCONST(ASTLeafFloat, -= );
			case fMul:	FOLDCONST(ASTLeafFloat, *= );
			case fDiv:	FOLDCONST(ASTLeafFloat, /= );
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