/*

1��C++����Lua��C����Lua���ƣ����ﲻ������
2��Lua����C++�е����Ա����Ա��������Ա��������ͨ����ʵ��ָ��

����ʵ��ָ���װ��lightUserData���ݵ�Lua��
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
	Cls():mem1(0){}
	int mem1;
public:
	int getMem1() { return mem1; }
	void setMem1(int n) { mem1 = n; }
};

Cls* pCls;//һ��ȫ�ֱ���

//������ӡlightUserData�洢��ָ��ֵ������ַ��
static int ForLua_printPointer(lua_State* L)
{
	const void* p = lua_topointer(L, 1);
	int pAddr = (int)p;
	cout << "ForLua_printPointer, p = " << pAddr << endl;
	return 0;
}

//Lua����C++,��ָ���C++����Lua
static int ForLua_getClsPointer(lua_State* L)
{
	if (pCls == NULL)
	{
		return 0;
	}

	lua_pushlightuserdata(L, pCls); //ע�������ǰ�ָ���װ��lightUserData���ݵ�Lua��
	return 1;
}

static int ForLua_getClsMem1(lua_State* L)
{
	Cls* p = (Cls*)lua_topointer(L, 1);
	int mem1 = pCls->getMem1();
	lua_pushnumber(L, mem1);

	return 1;
}

static int ForLua_setClsMem1(lua_State* L)
{
	Cls* p = (Cls*)lua_topointer(L, 1);
	int n = (int)lua_tointeger(L, 2);//�ڶ���������һ������

	pCls->setMem1(n);

	return 0;
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

	//1��������ע�ᵽLua��
	lua_pushcfunction(L, ForLua_printPointer);
	lua_setglobal(L, "printPointer");

	lua_pushcfunction(L, ForLua_getClsPointer);
	lua_setglobal(L, "getClsPointer");

	lua_pushcfunction(L, ForLua_getClsMem1);
	lua_setglobal(L, "getClsMem1");

	lua_pushcfunction(L, ForLua_setClsMem1);
	lua_setglobal(L, "setClsMem1");

	luaL_dofile(L, "patch.lua");

	cout << "in main, pCls->mem1 = " << pCls->getMem1() << endl;
}