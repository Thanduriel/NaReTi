#include <exception>
#include <string>

#include "token.h"

namespace par{
	//an exception thrown when an parsing error occurs.
	// The parsing process can not continue when an error in the parsed code is found.
	class ParsingError : public std::exception
	{
	public:
		ParsingError(const std::string& _msg, tok::Token* _token = nullptr) :
			message(_msg),
			token(_token)
		{};

		std::string message;
		tok::Token* token;
	};
}