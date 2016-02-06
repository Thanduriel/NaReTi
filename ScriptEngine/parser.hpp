#pragma once

#include "token.h"
#include "module.hpp"
#include "symbols.hpp"

#include "boostparser.hpp"
#include "semanticparser.hpp"

namespace par{
	class Parser
	{
	public:
		Parser();
		bool parse(const std::string& _text, NaReTi::Module& _module);

	private:
		//parsing pipline
		par::SemanticParser m_semanticParser;
		par::NaReTiGrammer m_grammer;
		par::NaReTiSkipper m_skipper;
	};

}