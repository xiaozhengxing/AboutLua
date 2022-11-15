/*

�͹���LuaCallCpp����
��patch.lua�еĴ��������һ����

����ʵ��ָ���װ��UserData���ݵ�Lua�У�ע�⣬�ڹ���LuaCallCpp�У��Ƿ�װ�� lightUserData��
ʹ��Userdata,ʹ��Lua�д�������������Ĵ��룺 p:SetMem1(100),�﷨����������C++
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
	//��ȡ��ʵ��ָ��
	Cls* pcls = (Cls*)lua_topointer(L, 1);

	//�½�UserData������ָ��洢��UserData��
	Cls** pData = (Cls**)lua_newuserdata(L, sizeof(Cls*));//����һ���ռ䣬�������(Cls*)�������ظÿռ�ĵ�ַ(Cls **)
	*pData = pcls;

	//����Ԫ��(һ��table)
	const char* clsName = lua_tostring(L, 2);//"Cls"
	luaL_getmetatable(L, clsName);

	//��Ԫ��ֵ��UserData��
	lua_setmetatable(L, -2);

	return 1;
}

static int ForLua_setMem1(lua_State* L)
{
	//ջ����Ԫ�ز���һ��lightUserdata�ˣ�����һ��UserData
	Cls* pcls = *(Cls**)lua_topointer(L, 1);//ע��͹���LuaCallCpp��Ĳ�һ����
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
	else if (str == "setMem1")//ע�⣬�˴�Ϊ�˼�㣬���ǽ������ͺ������ŵ�index�У���ʵ��������ֱ�ӷŵ�Ԫ������ͺ�
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

Cls* pCls;//һ��ȫ�ֱ���

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

	//����һ���µ�Ԫ�� mt1
	luaL_newmetatable(L, "Cls");

	//Ϊ������,���ｫ"__index"��Ӧһ��function.("__index"Ҳ���Զ�Ӧһ��Ԫ���������)
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, ForLua_index);
	lua_settable(L, -3);//��ʱ��ջ��{stack[-3]: Ԫ��mt1; stack[-2]: key; stack[-1]:value}���������mt1[key] = value������key��value��ջ

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, ForLua_newindex);
	lua_settable(L, -3);//��ʱ��ջ��{stack[-3]: Ԫ��mt1; stack[-2]: key; stack[-1]:value}���������mt1[key] = value������key��value��ջ

	lua_pop(L, 1);//��ǰ��newmetatable��ջ

	luaL_dofile(L, "patch.lua");

	cout << "in main, pCls->mem1 = " << pCls->getMem1() << endl;
}