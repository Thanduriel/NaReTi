#pragma once

#include <boost/config/warning_disable.hpp>
#include <boost/bind.hpp>

#include "attributecasts.hpp"
//testing / debugging
#include <iostream>
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


			//the actual grammer

			// by default declarations go to the global scope
			BaseExpression = 
				*((TypeDeclaration | FuncDeclaration | VarDeclaration)[boost::bind(&SemanticParser::resetScope, &m_semanticParser)]) >>
				qi::eoi;

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
				'{' >>
				*(GeneralExpression) >>
				'}'
				;

			GeneralExpression = 
				(("return" >> Expression)[boost::bind(&SemanticParser::returnStatement, &m_semanticParser)]
				| VarDeclaration
				| Expression)[boost::bind(&SemanticParser::finishGeneralExpression, &m_semanticParser)]
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

			Operand = 
				Symbol[boost::bind(&SemanticParser::pushSymbol, &m_semanticParser, ::_1)]
				| Integer[boost::bind(&SemanticParser::pushInt, &m_semanticParser, ::_1)]
				| Float[boost::bind(&SemanticParser::pushFloat, &m_semanticParser, ::_1)]
				| ConstString
				;

			// match any special char that can be used as an operator
			//pay attention that '-' needs to be the last char so that it is not interpreted as range
			Operator = lexeme[+char_("?+*/<>=|^%~!&.-")];
			
		//	Number = Integer;

			Integer = 
				int_ >>
				!char_('.') //cannot be followed by an . as that would indicate a float
				;
			Float = double_;
			/*
			Float = ;

			number = flt | integer;
			Operand = number |
				Var |
				('-' >> Operand) |
				('+' >> Operand)
				;*/
		}
	private:
		qi::rule<Iterator, Skipper> BaseExpression;
		qi::rule<Iterator, Skipper> TypeDeclaration;
		qi::rule<Iterator, Skipper> VarDeclaration;
		qi::rule<Iterator, Skipper> FuncDeclaration;

		qi::rule<Iterator, Skipper> Expression;
		qi::rule<Iterator, Skipper> GeneralExpression;
		qi::rule<Iterator, Skipper> RExpression;
		
		qi::rule<Iterator, std::string()> Symbol;
		qi::rule<Iterator, std::string()> ConstString;
		qi::rule<Iterator, int()> Integer;
		qi::rule<Iterator, double()> Float;
		qi::rule<Iterator, Skipper> Operand;
		qi::rule<Iterator, std::string()> Operator;

		par::SemanticParser& m_semanticParser;
	};

	typedef NaReTiSyntax<std::string::const_iterator, NaReTiSkipper> NaReTiGrammer;

}