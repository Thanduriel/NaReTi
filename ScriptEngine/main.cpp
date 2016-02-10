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
	cout << "module loaded: " << scriptEngine.loadMdoule("test.txt") << endl;
	NaReTi::FunctionHandle scriptHndl = scriptEngine.getFuncHndl("sum");
	cout << scriptEngine.call<int, int, int>(scriptHndl, 1, 3);

	char tmp;
	std::cin >> tmp;
	return 0;
}

