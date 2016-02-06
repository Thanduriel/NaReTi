#include "stdafx.h"
#include "lexer.hpp"


namespace tok{

	Lexer::Lexer()
	{
	}


	TokenizedText Lexer::tokenize(std::string&& _text)
	{
		std::vector < Token > tokens;
		std::string text(_text);

		unsigned int begin = 0;
		unsigned int end = 0;

		//state for multichar(word / number) reading
		int lookEnd = 0;

		//'.' count in numbers
		int count;
		bool isHex = false;

		unsigned int size = (unsigned int)text.size();

		for (unsigned int i = 0; i < size; ++i)
		{
			if (!lookEnd)
			{
				//probably a comment
				if (text[i] == '/')
				{
					//jump over the comment segment
					//a line
					if (text[i + 1] == '/')
					{
						i = (unsigned int)text.find('\n', i + 1);
						//no line break appears when this is the last line
						if (i == std::string::npos) i = size;
						//continue evaluating the segment after the comment
						continue;
					}
					//until comment is closed by "*/"
					// + 1 because the index of the first char is returned by find
					else if ((text[i + 1] == '*'))
					{
						i = (unsigned int)text.find("*/", i + 2) + 1;
						//continue evaluating the segment after the comment
						continue;
					}
					//division operator
					else if (text[i + 1] == '=')
					{
						begin = i;
						i++;
						tokens.emplace_back(TokenType::Operator, begin, i);
					}
					else tokens.emplace_back(TokenType::Operator, i, i);
				}

				//const string
				else if (text[i] == '"')
				{
					lookEnd = 3;
					begin = i;
				}

				//numeral begins
				else if (text[i] >= '0' && text[i] <= '9')
				{
					lookEnd = 2;

					//'.' count
					count = 0;
					begin = i;

					//hex number
					if (text[i] == '0' && text[i + 1] == 'x')
					{
						isHex = true;
						//jump over the prefix "0x"
						i += 2;
					}
				}

				//curly brackets
				else if (text[i] == '{') tokens.emplace_back(TokenType::CurlyBracketLeft, i, i);
				else if (text[i] == '}') tokens.emplace_back(TokenType::CurlyBracketRight, i, i);

				//parenthesis
				else if (text[i] == '(') tokens.emplace_back(TokenType::ParenthesisLeft, i, i);
				else if (text[i] == ')') tokens.emplace_back(TokenType::ParenthesisRight, i, i);

				//squarebracket
				else if (text[i] == '[') tokens.emplace_back(TokenType::SquareBracketLeft, i, i);
				else if (text[i] == ']') tokens.emplace_back(TokenType::SquareBracketRight, i, i);

				//comma
				else if (text[i] == ',') tokens.emplace_back(TokenType::Comma, i, i);


				//file structuring chars are skipped
				else if (text[i] == ' ' || text[i] == '\n' || text[i] == '\r' || text[i] == '\t')
				{
				}

				//string starts with a letter
				else if ((text[i] >= 0x41 && text[i] <= 0x5A) || (text[i] >= 0x61 && text[i] <= 0x7A) || text[i] == '_')
				{
					lookEnd = 1;
					//save index of the first char to return it later on
					begin = (unsigned int)i;
				}

				//anything else is an operator
				else
				{
					begin = (unsigned int)i;
					//some operators consist of two chars
					if (text[i + 1] == text[i] || text[i + 1] == '=')
						i++;

					//check for an unary minus
					auto it = tokens.end(); it--;
					//can not be an operator if not precedied by a symbol or constant
					if (text[i] == '-' && it->type != TokenType::Int && it->type != TokenType::Real && it->type != TokenType::Symbol)
					{
						text[i] = 'u'; // u for unary minus
					}

					tokens.emplace_back(TokenType::Operator, begin, (unsigned int)i);
				}
			}
			//lookEnd = 1 means that the end of a word is searched
			//any char that cannot be part of a symbol terminates the word
			else if (lookEnd == 1)
			{
				if (!(text[i] >= '0' && text[i] <= '9') && !(text[i] >= 0x41 && text[i] <= 0x5A) && !(text[i] >= 0x61 && text[i] <= 0x7A) && text[i] != '_')
				{
					//decrement the index counter so that it points to the last char of the word
					//causes revaluation of the termination char in the next iteration as it could be an operator
					i--;
					tokens.emplace_back(TokenType::Symbol, begin, i);

					//convert to lower case
					//todo do this only when caseSensitive == false
					for (unsigned int j = begin; j <= i; ++j)
						text[j] = ::tolower(text[j]);

					//return to default state
					lookEnd = 0;
				}
			}
			//numeral
			// no numchar or a second '.' terminate the numeral
			else if (lookEnd == 2)
			{
				if (text[i] == '.') count++;
				if (!(text[i] >= '0' && text[i] <= '9') && ((text[i] != '.') || (count > 1)))
				{
					//hex numbers allow A-F as letters
					if (isHex && text[i] >= 'A' && text[i] <= 'F') continue;

					i--;

					//check previous tokens as it might be a minus sign
					if (text[tokens.back().begin] == 'u')
					{
						//add the sign to the token
						begin--;
						tokens.pop_back();
					}
					//contains a '.' -> float
					tokens.emplace_back(count ? TokenType::Real : TokenType::Int, begin, i);

					lookEnd = 0;
				}
			}
			//const string
			//the '"' are discarded
			else if (lookEnd == 3 && text[i] == '"')
			{
				tokens.emplace_back(TokenType::String, begin + 1, i - 1);
				lookEnd = 0;
			}
		}

		return TokenizedText(std::move(tokens), std::move(text));
	}

}