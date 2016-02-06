#include "token.h"

namespace tok{
	int TokenizedText::getInt(Token& _token)
	{
		return strtol(&m_text[_token.begin], nullptr, 0);
	}

	// ********************************************* //

	float TokenizedText::getFloat(Token& _token)
	{
		return strtof(&m_text[_token.begin], nullptr);
	}

	// ********************************************* //

	std::string TokenizedText::getString(Token& _token)
	{
		return m_text.substr(_token.begin, _token.end + 1 - _token.begin);
	}

	bool TokenizedText::cmp(const Token& _tok, const std::string& _str)
	{ 
		return !m_text.compare(_tok.begin, 1 + _tok.end - _tok.begin, _str); 
	};
}