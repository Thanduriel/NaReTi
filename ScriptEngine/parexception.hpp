#include <exception>
#include <string>

namespace par{
	//an exception thrown when an parsing error occurs.
	// The parsing process can not continue when an error in the parsed code is found.
	class ParsingError : public std::exception
	{
	public:
		ParsingError(const std::string& _msg) :
			message(_msg)
		{};

		std::string message;
	};
}