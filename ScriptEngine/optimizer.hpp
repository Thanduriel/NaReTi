#pragma once

#include "module.hpp"

namespace codeGen{
	class Optimizer
	{
	public:
		//optimize the whole module
		void optimize(NaReTi::Module& _module);

		//restructures the tree reducing dept and tracing variable lifetimes
		void optimizeFunction(par::Function& _func);
	private:
	};
}