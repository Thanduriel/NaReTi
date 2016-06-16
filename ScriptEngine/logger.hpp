#pragma once

#include <iostream>
#include <string>
#include <sstream>

#define LOGLVL 0xFF

#define LOG(lvl, msg) do{std::ostringstream stream; \
 stream << "[" << logging::LogLvlNames[logging::lvl] << "] " << msg << std::endl; \
 logging::log(logging::lvl, stream.str());} while(false)

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

		clog << _msg;
	}
}