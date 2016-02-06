#pragma once

#include <string>
#include <vector>

namespace tok{
	enum TokenType
	{
		String,
		Symbol,
		Operator,
		Int,
		Real,
		CurlyBracketLeft,
		CurlyBracketRight,
		SquareBracketLeft,
		SquareBracketRight,
		ParenthesisLeft,
		ParenthesisRight,
		Comma
	};

	typedef unsigned int/*std::string::iterator*/ sourceIt;

	struct Token
	{
		Token(TokenType _type, sourceIt _begin = 0, sourceIt _end = 0)
			: type(_type),
			begin(_begin),
			end(_end)
		{};

		TokenType type;

		sourceIt begin;
		sourceIt end;

		//comparison operators
		bool operator==(const TokenType& _oth) { return type == _oth; };
		bool operator!=(const TokenType& _oth) { return type != _oth; };
	};

	//first translation step
	class TokenizedText
	{
		std::vector < Token > m_tokens;
		/*std::vector < Token >::iterator*/size_t m_iterator;

		std::string m_text; //still required to get the content

	public:
		TokenizedText() = default;

		// construction from all required data
		TokenizedText(std::vector < Token >&& _tokens, std::string&& _text) :
			m_tokens(_tokens),
			m_text(_text)
		{};

		//move constructor
		TokenizedText(TokenizedText&& _tokText) :
			m_tokens(std::move(_tokText.m_tokens)),
			m_text(std::move(_tokText.m_text)),
			m_iterator(_tokText.m_iterator)
		{};

		//token access
		Token& next() { return m_tokens[m_iterator++]; };
		Token& peek() { return m_tokens[m_iterator]; };
		Token& operator[](size_t _pos) { return m_tokens[_pos]; }; // token at a specific index

		//conversions
		int getInt(Token& _token);
		float getFloat(Token& _token);
		std::string getString(Token& _token);

		//some metadata
		size_t size() { return m_tokens.size(); };
		size_t pos() { return m_iterator; };

		//aditional comparison operators
		bool cmp(const Token& _tok, const std::string& _str);
	};
}