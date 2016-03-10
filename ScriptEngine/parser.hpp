#pragma once

#include "token.h"
#include "module.hpp"
#include "symbols.hpp"

#include "boostparser.hpp"
#include "preparser.hpp"

namespace par{
	class Parser
	{
	public:
		Parser();
		bool parse(const std::string& _text, NaReTi::Module& _module);
		void preParse(const std::string& _text, NaReTi::Module& _module);

		//per cpy
		std::vector<std::string> getDependencies() { return m_preParser.dependencies; }
	private:
		//parsing pipeline
		par::SemanticParser m_semanticParser;
		par::NaReTiGrammar m_grammar;
		par::PreParserGrammar m_preParserGrammar;
		PreParser m_preParser;
		par::NaReTiSkipper m_skipper;
	};

}