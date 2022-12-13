/*
参考
https://github.com/cloudwu/lua-snapshot
*/

#include <iostream>
#include <cassert>
#include <windows.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

extern "C"
{
#include "../Lua5_3_5/lua.h"
#include "../Lua5_3_5/lauxlib.h"
#include "../Lua5_3_5/lualib.h"
#include "../Lua5_3_5/ltable.h"
#include "../Lua5_3_5/lstate.h"
#include "../Lua5_3_5/lobject.h"
#include "../Lua5_3_5/lapi.h"
#include "../Lua5_3_5/lgc.h"

}

#define TABLE 1
#define FUNCTION 2
#define SOURCE 3
#define THREAD 4
#define USERDATA 5
#define MARK 6

static bool ismarked(lua_State* dL, const void* p)
{
	lua_rawgetp(dL, MARK, p);//Mark 索引处为一个table, 返回table[p], p可以看做是一个light userData
	if (lua_isnil(dL, -1))
	{
		lua_pop(dL, 1);
		lua_pushboolean(dL, 1);
		lua_rawsetp(dL, MARK, p);
		return false;
	}

	lua_pop(dL, 1);

	return true;
}

void mainSnapShot()
{
	lua_State* L = luaL_newstate();
	assert(L != NULL);

	luaopen_base(L);
	luaL_openlibs(L);

	int iRet = luaL_dofile(L, "SnapShot.lua");
	if (iRet)
	{
		cout << "load file SnapShot.lua error " << iRet << endl;
		const char* pErrorMsg = lua_tostring(L, -1);
		cout << pErrorMsg << endl;
		lua_close(L);

		return;
	}

	iRet = luaL_dofile(L, "DoSnapShot.lua");
	if (iRet)
	{
		cout << "load file DoSnapShot.lua error " << iRet << endl;
		const char* pErrorMsg = lua_tostring(L, -1);
		cout << pErrorMsg << endl;
		lua_close(L);

		return;
	}
}


void main()
{
	mainSnapShot();
}