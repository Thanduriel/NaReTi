#pragma once

#include <string>

#include "token.h"

namespace tok{

	class Lexer
	{
	public:
		Lexer();

		TokenizedText tokenize(std::string&& _text);
	};

}

