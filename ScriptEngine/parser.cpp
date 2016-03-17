#include <boost\spirit\home\classic.hpp>

#include "stdafx.h"
#include "parser.hpp"
#include "parexception.hpp"
#include "boostparser.hpp"

using namespace std;

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
		std::string::const_iterator iter = _text.begin();
		std::string::const_iterator end = _text.end();

		using boost::spirit::ascii::space;

		boost::spirit::qi::phrase_parse(iter, end, m_preParserGrammar, m_skipper);
	}

	// ************************************************** //

	bool Parser::parse(const std::string& _text, NaReTi::Module& _module)
	{
		m_semanticParser.setModule(_module);

		std::string::const_iterator iter = _text.begin();
		std::string::const_iterator end = _text.end();

		using boost::spirit::ascii::space;

		clock_t beginClock = clock();

		bool b = false;
		try
		{
			b = boost::spirit::qi::phrase_parse(iter, end, m_grammar, m_skipper);
		}
		catch (ParsingError& _error)
		{
			cout << _error.message << endl;
			return false;
		}
		catch (qi::expectation_failure<std::string::const_iterator>& e)
		{
			logError(_text.begin(), e.first, "syntax error");
		}

		clock_t endClock = clock();
		std::cout << endl << double(endClock - beginClock) / CLOCKS_PER_SEC << "sec" << endl;

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