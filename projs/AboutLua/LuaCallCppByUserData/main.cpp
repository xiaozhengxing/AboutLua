/*

和工程LuaCallCpp类似
但patch.lua中的代码操作不一样。

把类实例指针封装成UserData传递到Lua中（注意，在工程LuaCallCpp中，是封装成 lightUserData）
使用Userdata,使得Lua中代码可以有这样的代码： p:SetMem1(100),语法规则类似于C++
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

class Cls
{
public:
	Cls() :mem1(0) {}
	int mem1;
public:
	int getMem1() { return mem1; }
	void setMem1(int n) { mem1 = n; }
	void printMem1() { cout << "in printMem1, mem1 = " << mem1 << endl; }
};

static int ForLua_makeClsUserData(lua_State* L)
{
	//获取类实例指针
	Cls* pcls = (Cls*)lua_topointer(L, 1);

	//新建UserData，并将指针存储在UserData中
	Cls** pData = (Cls**)lua_newuserdata(L, sizeof(Cls*));//开辟一个空间，用来存放(Cls*)，并返回该空间的地址(Cls **)
	*pData = pcls;

	//查找元表(一个table)
	const char* clsName = lua_tostring(L, 2);//"Cls"
	luaL_getmetatable(L, clsName);

	//将元表赋值到UserData中
	lua_setmetatable(L, -2);

	return 1;
}

static int ForLua_setMem1(lua_State* L)
{
	//栈顶的元素不是一个lightUserdata了，而是一个UserData
	Cls* pcls = *(Cls**)lua_topointer(L, 1);//注意和工程LuaCallCpp里的不一样、
	int n = lua_tonumber(L, 2);

	pcls->setMem1(n);

	return 0;
}

static int ForLua_getMem1(lua_State* L)
{
	Cls* pcls = *(Cls**)lua_topointer(L, 1);
	lua_pushnumber(L, pcls->getMem1());

	return 1;
}

static int ForLua_printMem1(lua_State* L)
{
	Cls* pcls = *(Cls**)lua_topointer(L, 1);
	pcls->printMem1();

	return 0;
}

static int ForLua_index(lua_State* L)
{
	Cls* pcls = *(Cls**)lua_topointer(L, 1);
	string str = lua_tostring(L, 2);

	if (str == "mem1")
	{
		lua_pushnumber(L, pcls->mem1);
	}
	else if (str == "setMem1")//注意，此处为了简便，我们将变量和函数都放到index中，其实函数可以直接放到元表里面就好
	{
		lua_pushcfunction(L, ForLua_setMem1);
	}
	else if (str == "getMem1")
	{
		lua_pushcfunction(L, ForLua_getMem1);
	}
	else if (str == "printMem1")
	{
		lua_pushcfunction(L, ForLua_printMem1);
	}
	else
	{
		return 0;
	}

	return 1;
}

static int ForLua_newindex(lua_State* L)
{
	Cls* pcls = *(Cls**)lua_topointer(L, 1);
	string str = lua_tostring(L, 2);

	if (str == "mem1")
	{
		pcls->mem1 = (int)lua_tonumber(L, 3);
	}

	return 0;
}

Cls* pCls;//一个全局变量

static int ForLua_getClsPointer(lua_State* L)
{
	if (pCls == NULL)
	{
		return 0;
	}

	lua_pushlightuserdata(L, pCls);

	return 1;
}

void main()
{
	pCls = new Cls();
	cout << "in main, pCls = " << int(pCls) << endl;
	cout << "in main, pCls->mem1 = " << pCls->getMem1() << endl;

	lua_State* L = luaL_newstate();
	assert(L != NULL);

	luaopen_base(L);
	luaL_openlibs(L);

	lua_pushcfunction(L, ForLua_getClsPointer);
	lua_setglobal(L, "getClsPointer");

	lua_pushcfunction(L, ForLua_makeClsUserData);
	lua_setglobal(L, "makeClsUserData");

	//建立一个新的元表 mt1
	luaL_newmetatable(L, "Cls");

	//为简单明了,这里将"__index"对应一个function.("__index"也可以对应一个元表或者其他)
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, ForLua_index);
	lua_settable(L, -3);//此时堆栈中{stack[-3]: 元表mt1; stack[-2]: key; stack[-1]:value}。操作结果mt1[key] = value，并将key和value退栈

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, ForLua_newindex);
	lua_settable(L, -3);//此时堆栈中{stack[-3]: 元表mt1; stack[-2]: key; stack[-1]:value}。操作结果mt1[key] = value，并将key和value退栈

	lua_pop(L, 1);//把前面newmetatable出栈

	luaL_dofile(L, "patch.lua");

	cout << "in main, pCls->mem1 = " << pCls->getMem1() << endl;
}