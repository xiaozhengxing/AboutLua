/*

1、 Lua调用C
2、 C调用Lua

*/

#include <iostream>
#include <cassert>
#include <vector>

using namespace std;

extern "C"
{
#include "../Lua5_4_4/lua.h"
#include "../Lua5_4_4/lauxlib.h"
#include "../Lua5_4_4/lualib.h"
}

//C调用Lua中的函数
static int myAdd2(lua_State* L)
{
	double a = luaL_checknumber(L, 1);
	double b = luaL_checknumber(L, 2);

	double c = a + b + 1;

	lua_pushnumber(L, c);

	return 1;
}

void main()
{
	lua_State* L = luaL_newstate();
	assert(L != NULL);

	luaopen_base(L);
	luaL_openlibs(L);

	//1、Lua调用C
	lua_pushcfunction(L, myAdd2);
	lua_setglobal(L, "myAdd2");

	int bRet = luaL_dofile(L, "patch.lua");
	if (bRet)
	{
		cout << "load file error " << bRet << endl;
		return;
	}

	//2、C调用Lua
	lua_getglobal(L, "myAdd");
	lua_pushnumber(L, 10);
	lua_pushnumber(L, 20);
	int iRet = lua_pcall(L, 2, 1, 0);//2个参数，1个返回值

	if (iRet)
	{
		const char* pErrorMsg = lua_tostring(L, -1);
		cout << pErrorMsg << endl;
		lua_close(L);
		return;
	}

	if (lua_isnumber(L, -1))
	{
		double fValue = lua_tonumber(L, -1);
		cout << "C Call Lua， result = " << fValue << endl;
	}

	lua_close(L);
}