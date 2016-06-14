// ScriptEngine.cpp : Defines the entry point for the console application.
//

#define TEST(expression, message)      \
    if( !(expression) )                \
	    {                                  \
        std::cerr << "failed at:" << message << '\n';  \
        result = false;                \
	    }

#define FUNCHNDL(str) (scriptEngine.getFuncHndl(str))

#include "stdafx.h"
#include "../ScriptEngine/scriptengine.h"
#include <iostream>
#include <chrono>
#include <Windows.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "../ScriptEngine/complexalloc.hpp"

using namespace std;

int foo(int i)
{
	return i + 2;
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

bool testRun(NaReTi::ScriptEngine& scriptEngine)
{
	bool result = true;
	cout << "Doing unit tests: " << endl;
	TEST((scriptEngine.call<int, int, int>(FUNCHNDL("sum"), 40, 2) == 42), " basic int");
	TEST((scriptEngine.call<float, float, float>(FUNCHNDL("fsum"), 2.14015f, 1.00144f) == 2.14015f + 1.00144f), " basic float");
	TEST((scriptEngine.call<float, int, float>(FUNCHNDL("castTest"), 2, 0.71828f) == 2.f + 0.71828f), " typecast int -> float");

	NaReTi::FunctionHandle hndlMember = scriptEngine.getFuncHndl("memberAccess");
	Vec2 vect;
	vect.x = 7.f;
	vect.y = 42.5f;
	TEST((scriptEngine.call<float, Vec2*>(hndlMember, &vect) == vect.y), " member access");

	NaReTi::FunctionHandle hndlMemberAssign = scriptEngine.getFuncHndl("test_memberAssign");
	iVec2 ivec;
	ivec.x = 0;
	ivec.y = 2;
	scriptEngine.call<void, iVec2*>(hndlMemberAssign, &ivec);
	TEST((ivec.x == 42), " member assign");

	NaReTi::FunctionHandle hndlPrec = scriptEngine.getFuncHndl("precedence");
	TEST((scriptEngine.call<int, int, int>(hndlPrec, 2, 7) == 50), " precedence");

	NaReTi::FunctionHandle hndlLocalVar = scriptEngine.getFuncHndl("test_localVar");
	TEST((scriptEngine.call<int, int, int>(hndlLocalVar, 13, 11) == 13 + 11), " local  var");

	NaReTi::FunctionHandle hndlBranch = scriptEngine.getFuncHndl("test_branch");
	TEST((scriptEngine.call<int, int>(hndlBranch, 10) == 10 &&
		scriptEngine.call<int, int>(hndlBranch, 11) == 11 &&
		scriptEngine.call<int, int>(hndlBranch, 5) == 2), " branch");

	NaReTi::FunctionHandle hndlBool = scriptEngine.getFuncHndl("test_boolean");
	TEST((scriptEngine.call<int, int, int>(hndlBool, 0, 1) == 1
		&& scriptEngine.call<int, int, int>(hndlBool, 4, 0) == 1
		&& scriptEngine.call<int, int, int>(hndlBool, 6, 6) == 1
		&& scriptEngine.call<int, int, int>(hndlBool, 4, 1) == 0
		&& scriptEngine.call<int, int, int>(hndlBool, 0, 2) == 0), " boolean");

	NaReTi::FunctionHandle hndlLoop = scriptEngine.getFuncHndl("test_loop");
	TEST((scriptEngine.call<int, int, int>(hndlLoop, 3, 2) == 59), " loop");

	NaReTi::FunctionHandle hndlGlobalInit = scriptEngine.getFuncHndl("test_globalInit");
	scriptEngine.call<void>(hndlGlobalInit);

	TEST((scriptEngine.call<int, int>(FUNCHNDL("test_external"), 723) == 723+1+2), " linked external call");
	TEST(((scriptEngine.call<float, float>(FUNCHNDL("test_sqrt"), 765.34f) - 27.6647f) < 0.01f), " simple sqrt");
	TEST(((scriptEngine.call<float, float>(FUNCHNDL("test_sin"), 0.25f) - 0.247411f) < 0.01f), " fast sin");

	TEST((scriptEngine.call<int>(FUNCHNDL("test_optimRetVal"))), " return value optimization");
	return result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//	_CrtSetBreakAlloc(810);
	bool result = true;


	NaReTi::ScriptEngine scriptEngine;
	NaReTi::Config& config = scriptEngine.config();
	config.optimizationLvl = NaReTi::None;
	config.scriptLocation = "../scripts/";

	//some native function
	NaReTi::FunctionHandle hndl(foo);
	TEST((scriptEngine.call<int, int>(hndl, 5) == 7), "native call with handle");

	//link externals
	NaReTi::Module* externals = scriptEngine.getModule("externals.nrt");
	externals->linkExternal("printI", &printI);
	externals->linkExternal("printF", &printF);
	externals->linkExternal("printStr", &printStr);
	externals->linkExternal("getTickCount", &GetTickCount);
	externals->linkExternal("fooAdd2", &foo);

	bool success = scriptEngine.loadModule("unittest.nrt");
	if (!success) cout << "ERROR: Could not load unittest.nrt";
	else
	{
		cout << "Unit tests with optimization level none completed: " << testRun(scriptEngine) << endl;

		//unload dependencies to ensure that they are recompiled aswell
		config.optimizationLvl = NaReTi::Basic;
		scriptEngine.unloadModule("vector");
		scriptEngine.unloadModule("random");
		success = scriptEngine.reloadModule("unittest");

		if (!success) cout << "ERROR: Could not reload unittest.nrt" << endl;
		else cout << "Unit tests with optimization level basic completed: " << testRun(scriptEngine) << endl;
	}

	scriptEngine.loadModule("testing.nrt");

	//script functions
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


	char tmp;
	std::cin >> tmp;
	return 0;
}
