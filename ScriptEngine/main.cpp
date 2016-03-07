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

struct iVec2
{
	int x;
	int y;
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

		NaReTi::FunctionHandle hndlPrec = scriptEngine.getFuncHndl("precedence");
		cout << scriptEngine.call<int, int, int>(hndlPrec, 2, 7) << endl;

		NaReTi::FunctionHandle hndlLocalVar = scriptEngine.getFuncHndl("test_localVar");
		cout << scriptEngine.call<int, int, int>(hndlLocalVar, 13, 11) << endl;

		NaReTi::FunctionHandle hndlMemberAssign = scriptEngine.getFuncHndl("test_memberAssign");
		iVec2 ivec;
		ivec.x = 0;
		ivec.y = 2;
		scriptEngine.call<void, iVec2*>(hndlMemberAssign, &ivec);
		cout << ivec.x << endl;

		NaReTi::FunctionHandle hndlBranch = scriptEngine.getFuncHndl("test_branch");
		cout << scriptEngine.call<int, int>(hndlBranch, 10) << endl;
		cout << scriptEngine.call<int, int>(hndlBranch, 11) << endl;
		cout << scriptEngine.call<int, int>(hndlBranch, 5) << endl;

		NaReTi::FunctionHandle hndlLoop = scriptEngine.getFuncHndl("test_loop");
		cout << scriptEngine.call<int, int, int>(hndlLoop, 3, 2) << endl;
	}

	char tmp;
	std::cin >> tmp;
	return 0;
}

