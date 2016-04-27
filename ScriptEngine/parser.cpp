#include "iterator.hpp"
//#include <boost/spirit/classic_position_iterator.hpp>

#include "stdafx.h"
#include "parser.hpp"
#include "parexception.hpp"
#include "boostparser.hpp"

using namespace std;

str_it g_lastIterator;

namespace par{

	Parser::Parser():
		m_semanticParser(),
		m_grammar(m_semanticParser),
		m_preParserGrammar(m_preParser)
	{
	}

	// ************************************************** //

	void Parser::preParse(const std::string& _text, NaReTi::Module& _module)
	{
		m_preParser.dependencies.clear();

		pos_iterator_type posIt(_text.begin());
		pos_iterator_type posEnd(_text.end());

		g_lastIterator = _text.begin();
		boost::spirit::qi::phrase_parse(posIt, posEnd, m_preParserGrammar, m_skipper);
	}

	// ************************************************** //

	bool Parser::parse(const std::string& _text, NaReTi::Module& _module)
	{
		m_semanticParser.setModule(_module);

		g_lastIterator = _text.begin();

		pos_iterator_type posIt(_text.begin());
		pos_iterator_type posEnd(_text.end(), true);
		
		using boost::spirit::ascii::space;

		clock_t beginClock = clock();

		bool b = false;
		try
		{
			b = boost::spirit::qi::phrase_parse(posIt, posEnd, m_grammar, m_skipper);
		}
		catch (ParsingError& _error)
		{
			logError(_text.begin(), g_lastIterator, _error.message);
			return false;
		}
		catch (qi::expectation_failure<std::string::const_iterator>& e)
		{
			logError(_text.begin(), e.first, "syntax error");
		}

		clock_t endClock = clock();
		std::cout << endl << _module.m_name << " " << double(endClock - beginClock) / CLOCKS_PER_SEC << "sec" << endl;

		return b;
	}

	void Parser::logError(std::string::const_iterator _begin, const std::string::const_iterator& _it, const std::string& _msg)
	{
		int lineCount = 1; //lines are numbered beginning with 1
		while (_begin != _it)
		{
			if (*_begin == '\n') lineCount++;
			_begin++;
		}
		std::cout << "[l." << lineCount << "] " << _msg << endl;
	}
}