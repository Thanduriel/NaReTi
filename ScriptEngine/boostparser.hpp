#pragma once

#include <boost/config/warning_disable.hpp>
#include <boost/bind.hpp>

#include "attributecasts.hpp"
#include "semanticparser.hpp"
#include "skipper.hpp"


//using namespace boost::spirit;

namespace par
{
	namespace qi = boost::spirit::qi;

	template <typename Iterator, typename Skipper = CommentSkipper<Iterator>>
	struct NaReTiSyntax : qi::grammar<Iterator, Skipper>
	{
	public:
		NaReTiSyntax(par::SemanticParser& _parser) : NaReTiSyntax::base_type(BaseExpression),
			m_semanticParser(_parser)
		{
			using ascii::char_;


			//the actual grammar

			// by default declarations go to the global scope
			BaseExpression = 
				*UseStatement >>
				*((TypeDeclaration | FuncDeclaration | VarDeclaration)[boost::bind(&SemanticParser::resetScope, &m_semanticParser)]) >>
				qi::eoi;

			UseStatement =
				"use" >> Symbol
				;

			TypeDeclaration = 
				("type" >> Symbol)[boost::bind(&SemanticParser::typeDeclaration, &m_semanticParser, ::_1)] >>
				'{' >> 
				*VarDeclaration >> 
				'}';

			VarDeclaration = 
				(Symbol >> 
				-char_('&') >>
				Symbol)[boost::bind(&SemanticParser::varDeclaration, &m_semanticParser, ::_1)];

			//a simple string
			Symbol = (lexeme[char_("a-zA-Z_") >> *(char_("a-zA-Z_0-9"))]);

			//a quoted string
			ConstString = lexeme['"' >> +(char_ - '"') >> '"'];
			
			//type is optional
			FuncDeclaration = 
				(Symbol >> -Symbol >> '(')[boost::bind(&SemanticParser::funcDeclaration, &m_semanticParser, ::_1)] >>
				-VarDeclaration >> 
				*(',' >> VarDeclaration) >> 
				lit(')')[boost::bind(&SemanticParser::finishParamList, &m_semanticParser)] >>
				(lit("external")[boost::bind(&SemanticParser::makeExternal, &m_semanticParser)]
				| CodeScope)
				;

			GeneralExpression = 
				(("return" >> Expression)[boost::bind(&SemanticParser::returnStatement, &m_semanticParser)]
				| Conditional
				| Loop
				| VarDeclaration
				| Expression
				)[boost::bind(&SemanticParser::finishGeneralExpression, &m_semanticParser)]
				;

			//here we have terms...
			Expression = 
				(((('(' >> Expression >> ')')[boost::bind(&SemanticParser::lockLatestNode, &m_semanticParser)]
				| Operand) >>
				-RExpression)) //match with an operator
				;

			RExpression =
				(Operator >>
				(('(' >> Expression >> ')')[boost::bind(&SemanticParser::lockLatestNode, &m_semanticParser)]
				| Operand))[boost::bind(&SemanticParser::term, &m_semanticParser, ::_1)] >>
				-RExpression
				;

			CodeScope =
				lit('{')[boost::bind(&SemanticParser::beginCodeScope, &m_semanticParser)] >>
				*(GeneralExpression) >>
				lit('}')[boost::bind(&SemanticParser::finishCodeScope, &m_semanticParser)]
				;

			//classic if / if else / else
			//the "else" scope is created manually because logically an "else if" is an if inside the outer else
			Conditional =
				(lit("if") >> '(' >> Expression >> ')')[boost::bind(&SemanticParser::ifConditional, &m_semanticParser)] >>
				CodeScope >>
				-(lit("else")[boost::bind(&SemanticParser::elseConditional, &m_semanticParser)] >>
				(Conditional | CodeScope)[boost::bind(&SemanticParser::finishCodeScope, &m_semanticParser)])
				;

			Loop =
				(lit("while") >> '(' >> Expression >> ')')[boost::bind(&SemanticParser::loop, &m_semanticParser)] >>
				CodeScope
				;

	/*		Conditional =
				(lit("if") >> '(' >> Expression >> ')')[boost::bind(&SemanticParser::ifConditional, &m_semanticParser)] >>
				CodeScope >>
				*((lit("else if") >> '(' >> Expression >> ')')[boost::bind(&SemanticParser::elseifConditional, &m_semanticParser)] >>
				CodeScope) >>
				-(lit("else")[boost::bind(&SemanticParser::elseConditional, &m_semanticParser)] >>
				CodeScope)
				;*/

			Call =
				(Symbol >>
				'(')[boost::bind(&SemanticParser::call, &m_semanticParser, ::_1)] >>
				-(Expression[boost::bind(&SemanticParser::argSeperator, &m_semanticParser)] >> *(',' >> Expression[boost::bind(&SemanticParser::argSeperator, &m_semanticParser)])) >>
				')'
				;

			Operand = 
				Call
				| Symbol[boost::bind(&SemanticParser::pushSymbol, &m_semanticParser, ::_1)]
				| Integer[boost::bind(&SemanticParser::pushInt, &m_semanticParser, ::_1)]
				| Float[boost::bind(&SemanticParser::pushFloat, &m_semanticParser, ::_1)]
				| ConstString
				;

			//lexer definitions

			// match any special char that can be used as an operator
			//pay attention that '-' needs to be the last char so that it is not interpreted as range
			Operator = lexeme[+char_("?+*/<>=|^%~!&.-")];
			

			Integer = 
				int_ >>
				!char_('.') //cannot be followed by an . as that would indicate a float
				;
			Float = double_;
		}
	private:
		qi::rule<Iterator, Skipper> BaseExpression;
		qi::rule<Iterator, Skipper> TypeDeclaration;
		qi::rule<Iterator, Skipper> VarDeclaration;
		qi::rule<Iterator, Skipper> UseStatement;
		qi::rule<Iterator, Skipper> FuncDeclaration;

		qi::rule<Iterator, Skipper> Expression;
		qi::rule<Iterator, Skipper> GeneralExpression;
		qi::rule<Iterator, Skipper> RExpression;
		qi::rule<Iterator, Skipper> CodeScope;
		qi::rule<Iterator, Skipper> Conditional;
		qi::rule<Iterator, Skipper> Loop;
		qi::rule<Iterator, Skipper> Call;

		
		qi::rule<Iterator, std::string()> Symbol;
		qi::rule<Iterator, std::string()> ConstString;
		qi::rule<Iterator, int()> Integer;
		qi::rule<Iterator, double()> Float;
		qi::rule<Iterator, Skipper> Operand;
		qi::rule<Iterator, std::string()> Operator;

		par::SemanticParser& m_semanticParser;
	};

	typedef NaReTiSyntax<std::string::const_iterator, NaReTiSkipper> NaReTiGrammar;

}