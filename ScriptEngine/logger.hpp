#pragma once

#include <iostream>
#include <string>

namespace logging{
	enum LogLvl
	{
		Info,
		Error,
		Warning
	};

	const static char* LogLvlNames[] = { "Info", "Error", "Warning" };

	static void log(LogLvl _log, const std::string& _msg)
	{
		using namespace std;

		clog << "[" << LogLvlNames[_log] << "] " << _msg << endl;
	}
}