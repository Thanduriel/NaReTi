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

struct Vec2
{
	float x;
	float y;
};

int _tmain(int argc, _TCHAR* argv[])
{
	NaReTi::ScriptEngine scriptEngine;

	//test cases

	//some native function
	NaReTi::FunctionHandle hndl(foo);
	cout << (scriptEngine.call<int, int>(hndl, 5) == 7) << endl;

	//script functions
	bool success = scriptEngine.loadModule("test.txt");
	cout << "module loaded: " << success << endl;
	if (success)
	{
		NaReTi::FunctionHandle hndlI = scriptEngine.getFuncHndl("sum");
		NaReTi::FunctionHandle hndlF = scriptEngine.getFuncHndl("fsum");
		NaReTi::FunctionHandle hndlCast = scriptEngine.getFuncHndl("castTest");
		cout << (scriptEngine.call<int, int, int>(hndlI, 40, 2) == 42) << " basic int" << endl;
		cout << (scriptEngine.call<float, float, float>(hndlF, 2.14015f, 1.00144f) == 2.14015f + 1.00144f) << " basic float" << endl;
		cout << (scriptEngine.call<float, int, float>(hndlCast, 2, 0.71828f) == 2.f + 0.71828f) << " typecast" << endl;

		NaReTi::FunctionHandle hndlMember = scriptEngine.getFuncHndl("memberAccess");
		Vec2 vect;
		vect.x = 7.f;
		vect.y = 42.5f;
		cout << scriptEngine.call<float, Vec2*>(hndlMember, &vect) << endl;

	}

	char tmp;
	std::cin >> tmp;
	return 0;
}

