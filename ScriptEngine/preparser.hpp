#pragma once

#include <boost/config/warning_disable.hpp>
#include <boost/bind.hpp>

#include "attributecasts.hpp"
#include "semanticparser.hpp"
#include "skipper.hpp"
#include "iterator.hpp"

namespace par
{
	class PreParser
	{
	public:
		void useStatement(const std::string& _name) { dependencies.push_back(_name); };
		std::vector < std::string > dependencies;
	};

	namespace qi = boost::spirit::qi;

	template <typename Iterator, typename Skipper = CommentSkipper<Iterator>>
	struct PreParserSyntax : qi::grammar<Iterator, Skipper>
	{
	public:
		PreParserSyntax(par::PreParser& _parser) : PreParserSyntax::base_type(UseStatement),
			m_parser(_parser)
		{
			using ascii::char_;

			Symbol = (lexeme[char_("a-zA-Z_") >> *(char_("a-zA-Z_0-9"))]);

			UseStatement =
				*("use" >> Symbol[boost::bind(&PreParser::useStatement, &m_parser, ::_1)])
				;
		}
	private:
		qi::rule<Iterator, Skipper> UseStatement;
		qi::rule<Iterator, std::string()> Symbol;

		par::PreParser& m_parser;
	};

	typedef PreParserSyntax<pos_iterator_type, NaReTiSkipper> PreParserGrammar;

}