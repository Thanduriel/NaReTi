#pragma once

#include "module.hpp"

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
		void traceNode(par::ASTNode* _node);
		void traceBranch(par::ASTBranch& _node);
		void traceLoop(par::ASTLoop& _node);
		void traceCall(par::ASTCall& _node);
		void traceReturn(par::ASTReturn& _node);
		void traceMember(par::ASTMember& _node);
		void traceLeaf(par::ASTLeaf& _node);

		//try to substitute _target with _sub in the current frame
		void trySubstitution(par::VarSymbol& _target, par::VarSymbol& _sub);
		std::vector< par::VarSymbol** > m_usageStack; //< all vars 
		std::vector< par::VarSymbol* > m_tempPtrs;
		par::Function* m_function;
	};
}