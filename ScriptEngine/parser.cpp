#include "iterator.hpp"
//#include <boost/spirit/classic_position_iterator.hpp>

#include "stdafx.h"
#include "parser.hpp"
#include "parexception.hpp"
#include "boostparser.hpp"
#include "logger.hpp"

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
		catch (ParsingError& _error) //semantic errors
		{
			logError(_text.begin(), g_lastIterator, _error.message);
			return false;
		}
		catch (qi::expectation_failure<pos_iterator_type>& e)
		{
			logError(_text.begin(), e.first, string("Syntax error: did not expect \"") + *e.first + "\"");
			return false;
		}

		clock_t endClock = clock();
	//	std::cout << "[Info]" << "Compiled " << _module.m_name << " in " << double(endClock - beginClock) / CLOCKS_PER_SEC << "sec" << endl;
		logging::log(logging::Info1, "Parsed \"" + _module.m_name + "\" in " + std::to_string(double(endClock - beginClock) / CLOCKS_PER_SEC) + "sec");
		return b;
	}

	void Parser::logError(str_it _begin, const str_it& _it, const std::string& _msg)
	{
		int lineCount = 1; //lines are numbered beginning with 1
		str_it lastLb;
		while (_begin != _it)
		{
			if (*_begin == '\n')
			{
				lineCount++;
				lastLb = _begin; //effectively the begin of the error line
			}
			_begin++;
		}
		//look for the end of the current line
		str_it nextLb(lastLb); //start at last endl because _it might be an '\n'
		while (*++nextLb != '\n');

		logging::log(logging::Error, "[l." + std::to_string(lineCount) + "] " + _msg + '\n' + string(++lastLb, nextLb));
//		std::clog << "[error]" <<"[l." << lineCount << "] " << _msg << endl;
//		std::clog << string(++lastLb, nextLb) << endl;
	}
}