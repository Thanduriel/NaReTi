#pragma once

#include "module.hpp"
#include "symbols.hpp"

#include "boostparser.hpp"
#include "preparser.hpp"

namespace par{
//	extern Parser g_dynamicParser; // parser to parse generated snippets

	class Parser
	{
	public:
		Parser();
		bool parse(const std::string& _text, NaReTi::Module& _module);
		void preParse(const std::string& _text, NaReTi::Module& _module);

		//per cpy
		std::vector<std::string> getDependencies() { return m_preParser.dependencies; }
	private:
		//writes an formated error message to the log including the line number and a snippet of the faulty code
		//@param _begin begin iterator to the text
		//@param _it iterator of where the error appeared
		void logError(str_it _begin, const str_it& _it, const std::string& _msg);

		//parsing pipeline
		par::SemanticParser m_semanticParser;
		par::NaReTiGrammar m_grammar;
		par::PreParserGrammar m_preParserGrammar;
		PreParser m_preParser;
		par::NaReTiSkipper m_skipper;
	};

}