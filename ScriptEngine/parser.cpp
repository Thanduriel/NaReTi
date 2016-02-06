#include <boost\spirit\home\classic.hpp>

#include "stdafx.h"
#include "parser.hpp"
#include "parexception.hpp"
#include "boostparser.hpp"

using namespace std;

namespace par{

	Parser::Parser():
		m_semanticParser(),
		m_grammer(m_semanticParser)
	{
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
			b = boost::spirit::qi::phrase_parse(iter, end, m_grammer, m_skipper);
		}
		catch (ParsingError& _error)
		{
			std::cout << _error.message;
			return false;
		}
		clock_t endClock = clock();
		std::cout << endl << double(endClock - beginClock) / CLOCKS_PER_SEC << "sec";

		return b;
	}
}