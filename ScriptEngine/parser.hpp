#pragma once

#include "module.hpp"
#include "symbols.hpp"

#include <boost/config/warning_disable.hpp>
#include "boostparser.hpp"
#include "preparser.hpp"

namespace par{

	class BasicParser
	{
	public:
		BasicParser();
		bool parse(const std::string& _text, NaReTi::Module& _module);
	protected:
		//writes an formated error message to the log including the line number and a snippet of the faulty code
		//@param _begin begin iterator to the text
		//@param _it iterator of where the error appeared
		void parsingError(str_it _begin, const str_it& _it, const str_it& _end, const std::string& _msg);

		par::SemanticParser m_semanticParser;
		par::NaReTiGrammar m_grammar;
		par::NaReTiSkipper m_skipper;
	};

	class Parser : public BasicParser
	{
	public:
		Parser();
		void preParse(const std::string& _text, NaReTi::Module& _module);

		//per cpy
		std::vector<std::string> getDependencies() { return m_preParser.dependencies; }
	private:
		
		par::PreParserGrammar m_preParserGrammar;
		PreParser m_preParser;
	};

}