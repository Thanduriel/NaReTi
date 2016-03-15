#include "optimizer.hpp"

namespace codeGen{
	const int InlineTreshhold = 5;

	void Optimizer::optimize(NaReTi::Module& _module)
	{
		for (auto& func : _module.m_functions)
			optimizeFunction(*func);
	}

	void Optimizer::optimizeFunction(par::Function& _func)
	{
		if (_func.scope.size() < InlineTreshhold) _func.bInline = true;

	}
}