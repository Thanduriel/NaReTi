// ScriptEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "scriptengine.hpp"
#include <iostream>
#include <chrono>
#include <windows.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "complexalloc.hpp"

using namespace std;

int foo(int i)
{
	std::cout << "foo got called with arg: " << i << std::endl;
	return i+2;
}

void printI(int num)
{
	cout << num << endl;
}

void printF(float num)
{
	cout << num << endl;
}

void printStr(char* _str)
{
	cout << _str << endl;
}

struct Vec2
{
	float x;
	float y;
};

struct Vec3
{
	float x;
	float y;
	float z;
};

struct iVec2
{
	int x;
	int y;
};

int _tmain(int argc, _TCHAR* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetBreakAlloc(810);

/*	NaReTi::ScriptEngine scriptEngine;
	NaReTi::Config& config = scriptEngine.config();
	config.optimizationLvl = NaReTi::Basic;
	config.scriptLocation = "scripts/";

	//test cases

	//some native function
	NaReTi::FunctionHandle hndl(foo);
	cout << (scriptEngine.call<int, int>(hndl, 5) == 7) << endl;

	//link externals
	NaReTi::Module* externals = scriptEngine.getModule("externals.nrt");
	externals->linkExternal("printI", &printI);
	externals->linkExternal("printF", &printF);
	externals->linkExternal("printStr", &printStr);
	externals->linkExternal("getTickCount", &GetTickCount);

	//script functions
	bool success = scriptEngine.loadModule("test.txt");
	cout << "module loaded: " << success << endl;
	if (success)
	{
		cout << "Doing unit tests: " << endl;
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
		cout << (scriptEngine.call<float, Vec2*>(hndlMember, &vect) == vect.y) << " member access" << endl;

		NaReTi::FunctionHandle hndlMemberAssign = scriptEngine.getFuncHndl("test_memberAssign");
		iVec2 ivec;
		ivec.x = 0;
		ivec.y = 2;
		scriptEngine.call<void, iVec2*>(hndlMemberAssign, &ivec);
		cout << (ivec.x == 42) << " member assign" << endl;

		NaReTi::FunctionHandle hndlPrec = scriptEngine.getFuncHndl("precedence");
		cout << (scriptEngine.call<int, int, int>(hndlPrec, 2, 7) == 50) << " precedence" << endl;

		NaReTi::FunctionHandle hndlLocalVar = scriptEngine.getFuncHndl("test_localVar");
		cout << (scriptEngine.call<int, int, int>(hndlLocalVar, 13, 11) == 13 + 11) << " local  var" << endl;

		NaReTi::FunctionHandle hndlBranch = scriptEngine.getFuncHndl("test_branch");
		cout << (scriptEngine.call<int, int>(hndlBranch, 10) == 10 &&
			scriptEngine.call<int, int>(hndlBranch, 11) == 11 && 
			scriptEngine.call<int, int>(hndlBranch, 5) == 2) << " branch" << endl;

		NaReTi::FunctionHandle hndlBool = scriptEngine.getFuncHndl("test_boolean");
		cout << (scriptEngine.call<int, int, int>(hndlBool, 0, 1) == 1
			&& scriptEngine.call<int, int, int>(hndlBool, 4, 0) == 1
			&& scriptEngine.call<int, int, int>(hndlBool, 6, 6) == 1
			&& scriptEngine.call<int, int, int>(hndlBool, 4, 1) == 0
			&& scriptEngine.call<int, int, int>(hndlBool, 0, 2) == 0) << " boolean" << endl;

		NaReTi::FunctionHandle hndlLoop = scriptEngine.getFuncHndl("test_loop");
		cout << (scriptEngine.call<int, int, int>(hndlLoop, 3, 2) == 59) << " loop" << endl;

		NaReTi::FunctionHandle hndlGlobalInit = scriptEngine.getFuncHndl("test_globalInit");
		scriptEngine.call<void>(hndlGlobalInit);
		NaReTi::FunctionHandle hndlGlobal = scriptEngine.getFuncHndl("test_global");
		cout << scriptEngine.call<int>(hndlGlobal) << " global" << endl;

		NaReTi::FunctionHandle hndlRndInit = scriptEngine.getFuncHndl("initRandom");
		scriptEngine.call<void, int>(hndlRndInit, 0x152351);
		NaReTi::FunctionHandle hndlRnd = scriptEngine.getFuncHndl("rand");
		int sum = 0;
		for (int i = 0; i < 1000; ++i) sum += scriptEngine.call<int, int, int>(hndlRnd, 15, 0);
		cout << sum / 1000 << " random" << endl;

		//performance test:
		NaReTi::FunctionHandle hndlPerf = scriptEngine.getFuncHndl("test_performance");
		sum = 0;
		for (int i = 0; i < 132; ++i) sum += scriptEngine.call<int>(hndlPerf);
		cout << sum / 132 << " performance" << endl;

		NaReTi::FunctionHandle hndlMain = scriptEngine.getFuncHndl("main");
		scriptEngine.call<void>(hndlMain);

	}
	*/
	char tmp;
	std::cin >> tmp;
	return 0;
}

