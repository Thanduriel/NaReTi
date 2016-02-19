// ScriptEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "scriptengine.h"
#include <iostream>

using namespace std;

int foo(int i)
{
	std::cout << "foo got called with arg: " << i << std::endl;
	return i+2;
}

int _tmain(int argc, _TCHAR* argv[])
{
	NaReTi::ScriptEngine scriptEngine;

	//some native function
	NaReTi::FunctionHandle hndl(foo);
	cout << scriptEngine.call<int, int>(hndl, 5) << endl;

	//a script func!!!
	cout << "module loaded: " << scriptEngine.loadModule("test.txt") << endl;
	NaReTi::FunctionHandle hndlI = scriptEngine.getFuncHndl("sum");
	NaReTi::FunctionHandle hndlF = scriptEngine.getFuncHndl("fsum");
	cout << scriptEngine.call<int, int, int>(hndlI, 40, 2) << endl;
	cout << scriptEngine.call<float, float, float>(hndlF, 2.14, 1.0015) << endl;

	char tmp;
	std::cin >> tmp;
	return 0;
}

