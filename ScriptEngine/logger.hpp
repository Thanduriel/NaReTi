#pragma once

#include <iostream>
#include <string>

#define LOGLVL 0xFF

namespace logging{
	enum LogLvl
	{
		Error,
		Warning,
		Info0, //highest priority info
		Info1,
	};

	const static char* LogLvlNames[] = { "Error", "Warning", "Info", "Info" };

	static void log(LogLvl _log, const std::string& _msg)
	{
		using namespace std;

		if ((int)_log >= LOGLVL) return; // log level is to low

		clog << "[" << LogLvlNames[_log] << "] " << _msg << endl;
	}
}