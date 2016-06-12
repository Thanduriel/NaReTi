#pragma once

#include <vector>
#include "parpredecs.hpp"

namespace NaReTi{
	class Module;
}

namespace codeGen{
	class Optimizer
	{
	public:
		Optimizer();
		//optimize the whole module
		void optimize(NaReTi::Module& _module);

		//restructures the tree reducing dept and tracing variable lifetimes
		void optimizeFunction(par::Function& _func);

		void traceCode(par::ASTCode& _node);
	private:
		//will call the correct trace function on the given node
		void traceNode(par::ASTNode* _node, par::ASTExpNode** _dest = nullptr);
		void traceBranch(par::ASTBranch& _node);
		void traceLoop(par::ASTLoop& _node);
		void traceCall(par::ASTCall& _node, par::ASTExpNode** _dest);
		void traceReturn(par::ASTReturn& _node);
		void traceMember(par::ASTMember& _node);
		void traceLeaf(par::ASTExpNode& _node); // is an ASTLeafSym

		void tryConstFold(par::ASTCall& _node, par::ASTExpNode** _dest);

		//try to substitute _target with _sub in the current frame(all previous occurrences)
		void trySubstitution(par::VarSymbol& _target, par::VarSymbol& _sub);
		std::vector< par::VarSymbol** > m_usageStack; //< all vars 
		std::vector< par::VarSymbol* > m_tempPtrs;
		par::Function* m_function;
		NaReTi::Module* m_module;
		int m_callCount;
	};
}