#pragma once

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
				*(TypeDeclaration						[boost::bind(&SemanticParser::resetScope, &m_semanticParser)]
				| FuncDeclaration						[boost::bind(&SemanticParser::resetScope, &m_semanticParser)]
				| VarDeclaration						[boost::bind(&SemanticParser::finishGeneralExpression, &m_semanticParser)]
				)>
				qi::eoi;

			UseStatement =
				"use" >> Symbol
				;

			TypeDeclaration = 
				("type" >> -GenericTypeParam >> Symbol)	[boost::bind(&SemanticParser::typeDeclaration, &m_semanticParser, ::_1)] >>
				'{' >> 
				*(VarDeclaration 
				| Constructor							
				| Destructor							
				) >>
				lit('}')								[boost::bind(&SemanticParser::finishTypeDec, &m_semanticParser)];

			//var declaration with optional init: int a = 10
			// and export keyword
			VarDeclaration =
				(TypeInformation >>Symbol)				[boost::bind(&SemanticParser::varDeclaration, &m_semanticParser, ::_1)] >>
				-((lit("=")								[boost::bind(&SemanticParser::pushLatestVar, &m_semanticParser)] >>
				Expression)								[boost::bind(&SemanticParser::finishInit, &m_semanticParser)]
				| (lit(":=")							[boost::bind(&SemanticParser::pushLatestVar, &m_semanticParser)] >>
				Expression)								[boost::bind(&SemanticParser::term, &m_semanticParser, string(":="))]
				| ('(' >> ArgList >> ')')
				) >> - lit("export")					[boost::bind(&SemanticParser::makeExport, &m_semanticParser)]
				;
			
			//func declaration including overloaded operators with keywords
			FuncDeclaration = 
				((TypeInformation >> 
				(Symbol | Operator | qi::string("[]")) >> 
				'(')									[boost::bind(&SemanticParser::funcDeclaration, &m_semanticParser, ::_1)] >
				FuncBody)								[boost::bind(&SemanticParser::finishFunction, &m_semanticParser)]
				;

			FuncBody = -VarDeclaration >>
				*(',' > VarDeclaration) >>
				lit(')')								[boost::bind(&SemanticParser::finishParamList, &m_semanticParser)] >>
				(lit("external")						[boost::bind(&SemanticParser::makeExternal, &m_semanticParser)]
				| CodeScope)
				;

			Constructor =
				lit("constructor")						[boost::bind(&SemanticParser::constructorDec, &m_semanticParser)] >>
				lit('(') >> FuncBody
				;

			Destructor =
				lit("destructor")						[boost::bind(&SemanticParser::destructorDec, &m_semanticParser)] >>
				lit('(') >> FuncBody
				;


			GenericTypeParam =
				'<' >>
				-Symbol									[boost::bind(&SemanticParser::genericTypePar, &m_semanticParser, ::_1)] >>
				*(Symbol >> lit(','))					[boost::bind(&SemanticParser::genericTypePar, &m_semanticParser, ::_1)] >>
				'>'
				;

			TypeInformation =
			//	*TypeAttr >>
				Symbol									[boost::bind(&SemanticParser::newTypeInfo, &m_semanticParser, ::_1)] >>
				-GenericTypeParam >>
				*TypeAttr
				;

			TypeAttr =
				lit("const")							[boost::bind(&SemanticParser::makeConst, &m_semanticParser)]
				| lit('&')								[boost::bind(&SemanticParser::makeReference, &m_semanticParser)]
				| lit('[') >>
				Integer									[boost::bind(&SemanticParser::setArraySize, &m_semanticParser, ::_1)] >>
				lit(']')								[boost::bind(&SemanticParser::makeArray, &m_semanticParser)]
				;

			GeneralExpression =
				(("return" >> (lit(";") | Expression))	[boost::bind(&SemanticParser::returnStatement, &m_semanticParser)]
				| Conditional
				| Loop
				| VarDeclaration
				| Expression
				)										[boost::bind(&SemanticParser::finishGeneralExpression, &m_semanticParser)]
				;

			//mathematical terms including brackets"()"; 
			//function calls: foo(args); square bracket: a[]
			Expression = 
				LExpression >
				-RExpression
				;

			LExpression =
				(-Operator >>
				(('(' >> Expression >> ')')				[boost::bind(&SemanticParser::lockLatestNode, &m_semanticParser)]
				| Operand) >> 
				(-('[' >> Expression >> ']')			[boost::bind(&SemanticParser::term, &m_semanticParser, string("[]"))])
				)										[boost::bind(&SemanticParser::unaryTerm, &m_semanticParser, ::_1)]
				;

			//operator and operand
			RExpression =
				((Operator >
				LExpression)							[boost::bind(&SemanticParser::term, &m_semanticParser, ::_1)]) >>
				-RExpression
				;

			CodeScope =
				(lit('{')								[boost::bind(&SemanticParser::beginCodeScope, &m_semanticParser)] >
				*(GeneralExpression) >
				lit('}')								[boost::bind(&SemanticParser::finishCodeScope, &m_semanticParser)]
				) | GeneralExpression
				;

			//classic if / if else / else
			//the "else" scope is created manually because logically an "else if" is an if inside the outer else
			Conditional =
				(lit("if") >> '(' >> Expression >> ')')	[boost::bind(&SemanticParser::ifConditional, &m_semanticParser)] >>
				CodeScope >>
				-(lit("else")							[boost::bind(&SemanticParser::elseConditional, &m_semanticParser)] >>
				(Conditional | CodeScope)				[boost::bind(&SemanticParser::finishCodeScope, &m_semanticParser)])
				;

			Loop =
				(lit("while") >> 
				'(' >> Expression >> ')')				[boost::bind(&SemanticParser::loop, &m_semanticParser)] >>
				CodeScope
				;

			Call =
				(Symbol >>
				'(')									[boost::bind(&SemanticParser::call, &m_semanticParser, ::_1)] >>
				ArgList >>
				')'
				;
			ArgList =
				-(Expression							[boost::bind(&SemanticParser::argSeperator, &m_semanticParser)] >>
				*(',' >> Expression						[boost::bind(&SemanticParser::argSeperator, &m_semanticParser)]
				))
				;

			Operand = 
				(lit("sizeof") >> lit('(') >>
				TypeInformation >> lit(')'))			[boost::bind(&SemanticParser::sizeOf, &m_semanticParser)]
				| Call
				| Symbol								[boost::bind(&SemanticParser::pushSymbol, &m_semanticParser, ::_1)]
				| Address								[boost::bind(&SemanticParser::pushAddress, &m_semanticParser, ::_1)]
				| Integer								[boost::bind(&SemanticParser::pushInt, &m_semanticParser, ::_1)]
				| Float									[boost::bind(&SemanticParser::pushFloat, &m_semanticParser, ::_1)]
				| ConstString							[boost::bind(&SemanticParser::pushString, &m_semanticParser, ::_1)]
				;

			//lexer definitions

			//a simple string
			Symbol = (lexeme[char_("a-zA-Z_") >> *(char_("a-zA-Z_0-9"))]);

			//a quoted string
			ConstString = lexeme[lit('"') >> +(char_ - '"') >> lit('"')];

			// match any special char that can be used as an operator
			//pay attention that '-' needs to be the last char so that it is not interpreted as range
			Operator = lexeme[+char_("?:'+*/<>=|^%~!&.-")];
			

			Integer = 
				int_ >>
				!char_('.') //cannot be followed by an . as that would indicate a float
				;
			Float = double_;

			Address =
				lit("0a") >>
				hex64
				;


//			qi::on_error<qi::fail>(BaseExpression, handler);
		}
	private:
/*		static void handler(boost::fusion::vector< Iterator&, Iterator const&, Iterator const&, std::string&> args)
		{
			cout << "bla";
		}*/

		qi::rule<Iterator, Skipper> BaseExpression;
		qi::rule<Iterator, Skipper> TypeDeclaration;
		qi::rule<Iterator, Skipper> VarDeclaration;
		qi::rule<Iterator, Skipper> UseStatement;
		qi::rule<Iterator, Skipper> FuncDeclaration;
		qi::rule<Iterator, Skipper> FuncBody;

		qi::rule<Iterator, Skipper> Constructor;
		qi::rule<Iterator, Skipper> Destructor;
		qi::rule<Iterator, Skipper> TypeInformation;
		qi::rule<Iterator, Skipper> TypeAttr;
		qi::rule<Iterator, Skipper> GenericTypeParam;

		qi::rule<Iterator, Skipper> Expression;
		qi::rule<Iterator, Skipper> GeneralExpression;
		qi::rule<Iterator, Skipper> LExpression;
		qi::rule<Iterator, Skipper> RExpression;
		qi::rule<Iterator, Skipper> CodeScope;
		qi::rule<Iterator, Skipper> Conditional;
		qi::rule<Iterator, Skipper> Loop;
		qi::rule<Iterator, Skipper> Call;
		qi::rule<Iterator, Skipper> ArgList;
		
		qi::rule<Iterator, std::string()> Symbol;
		qi::rule<Iterator, std::string()> ConstString;
		qi::rule<Iterator, int()> Integer;
		qi::rule<Iterator, double()> Float;
		qi::rule<Iterator, uint64_t()> Address;
		qi::rule<Iterator, Skipper> Operand;
		qi::rule<Iterator, std::string()> Operator;

		qi::uint_parser<uint64_t, 16, 1, 16> hex64;

		par::SemanticParser& m_semanticParser;
	};
	
	typedef NaReTiSyntax<pos_iterator_type, NaReTiSkipper> NaReTiGrammar;

}