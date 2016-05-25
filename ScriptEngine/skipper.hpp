#pragma once

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <string>
#include "iterator.hpp"

namespace par{
	using namespace boost::spirit;
	namespace qi = boost::spirit::qi;

	template<typename Iterator>
	struct CommentSkipper : public qi::grammar<Iterator> {

		/* valid comments are:
		 * "//" until end of line
		 * "/*" until the matching "* /"(without space) (this can be recursive)
		 */
		CommentSkipper() : CommentSkipper::base_type(Skip) {
			using standard::char_;

			Skip = standard::space | (ascii::string("//") >> *(char_ - '\n')) | VariableComment;
			VariableComment = ascii::string("/*") >> *(VariableComment | (char_ - ascii::string("*/"))) >> ascii::string("*/");
		}
		qi::rule<Iterator> Skip;
		qi::rule<Iterator> VariableComment;
	};

	template<typename Iterator>
	struct NoNewLineSkipper : public qi::grammar<Iterator> {

		NoNewLineSkipper() : NoNewLineSkipper::base_type(Skip) {
			using ascii::char_;

			Skip = (ascii::space - qi::eol) | (ascii::string("//") >> *(char_ - '\n')) | VariableComment;
			VariableComment = ascii::string("/*") >> *(VariableComment | (char_ - ascii::string("*/"))) >> ascii::string("*/");
		}
		qi::rule<Iterator> Skip;
		qi::rule<Iterator> VariableComment;
	};

	typedef CommentSkipper< pos_iterator_type > NaReTiSkipper;
}