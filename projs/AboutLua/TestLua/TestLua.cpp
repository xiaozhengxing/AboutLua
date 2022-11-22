/*

1、 Lua调用C
2、 C调用Lua

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
#include "../Lua5_3_5/llex.h"

}
#define gnodelast(h) gnode(h, cast(size_t, sizenode(h)))

enum  RelationShipType
{
    RelationShipType_TableValue1 = 1,//table[key]是另一个table, key为string或其他类型
    RelationShipType_NumberKeyTableValue2 = 2,//table[key]是另一个table, key为number(1数组array里面的值, 2 key本身就是数字)
    RelationShipType_KeyOfTable3 = 3,//table[key]时, key是另一个table
    RelationShipType_Metatable4 = 4,//table的元表metatable
    RelationShipType_UpVALUE5 = 5,//upvalue of closure,闭包的上值
};

static int table_size(Table *h, int fast)
{
    if(fast)
    {
#if LUA_VERSION_NUM >= 504
        return sizenode(h) + h->alimit;
#else
        return sizenode(h) + h->sizearray;    
#endif
    }
    else
    {
        Node * n, *limit = gnodelast(h);
        int i = luaH_getn(h);
        for(n = gnode(h, 0); n < limit; n++)
        {
            if(!ttisnil(gval(n)))
            {
                i++;
            }
        }
        return i;
    }
}

class Data;
struct RefInfo;
typedef void (*TableSizeReport) (Data &data, const void *p, int size);
typedef void (*ObjectRelationshipReport) (map<intptr_t, vector<RefInfo>> &result, const void *parent, const void *child, RelationShipType type, const char * key, double d, const char *key2);


//从根节点开始,遍历整个链表,如果是table, 执行函数cb
//最终: data.TableSizes[(intptr_p)h] = table_size(h, fast)
void xlua_report_table_size(Data &data, lua_State *L, TableSizeReport cb, int fast)
{
    GCObject *p =G(L)->allgc;
    while(p != NULL)
    {
        if(p->tt == LUA_TTABLE)
        {
            Table *h = gco2t(p);
            cb(data, h, table_size(h, fast));//data.TableSizes[(intptr_p)h] = table_size(h, fast)
        }
        p = p->next;
    }
}

//遍历表table中的所有key-value,只要key-value有任意一个是table,则执行函数cb
void report_table(map<intptr_t, vector<RefInfo>> &result, Table *h, ObjectRelationshipReport cb)
{
    Node *n, *limit = gnodelast(h);
    unsigned int i;

    if(h->metatable != NULL)
    {
        cb(result, h, h->metatable, RelationShipType_Metatable4, NULL, 0, NULL);//处理table的元表metatable
    }
#if LUA_VERSION_NUM >= 504
    for(int i = 0; i < h->alimit; i++)
#else
    for(int i = 0; i < h->sizearray; i++)
#endif
    {
        const TValue *item = &h->array[i];
        if(ttistable(item))
        {
            cb(result, h, gcvalue(item), RelationShipType_NumberKeyTableValue2, NULL, i + 1, NULL);//table中的数组部分里面存在table时
        }
    }

    for(n =gnode(h, 0); n < limit; n++)
    {
        if(!ttisnil(gval(n)))
        {
#if LUA_VERSION_NUM >= 504
            const TValue *key = (const TValue *)&(n->u.key_val);
#else
            const TValue *key = gkey(n);
#endif
            if(ttistable(key))
            {
                cb(result, h, gcvalue(key), RelationShipType_KeyOfTable3, NULL, 0, NULL);//table中的散列表部分, node.key是一个table   
            }
            
            bool b = false;
            if(gcvalue(key) != NULL && b == false)
            {
                GCObject *obj = gcvalue(key);
                if(obj->tt == 4)
                {
                    string s = getstr(gco2ts(obj));
                    cout << s << endl;
                }
            }

            if(ttisstring(key))//key为string
            {
                cout << "key=" << getstr(tsvalue(key)) << endl;
            }
            
            const TValue *value = gval(n);
            if(ttistable(value))//value为table
            {
                if(ttisstring(key))//key为string
                {
                    cb(result, h, gcvalue(value), RelationShipType_TableValue1, getstr(tsvalue(key)), 0, NULL);//table中的散列表部分val=node[key]是一个table, key为string
                }
                else if(ttisnumber(key))//key为number
                {
                    cb(result, h, gcvalue(value), RelationShipType_NumberKeyTableValue2, NULL, nvalue(key), NULL);//table中的散列表部分val=node[key]是一个table, key为number
                }
                else//key为其他类型
                {
#if LUA_VERSION_NUM >= 504
                    cb(result, h, gcvalue(value), RelationShipType_TableValue1, NULL, novariant(key->tt_), NULL);//table中的散列表部分val=node[key]是一个table, key为其他类型, 这里传入key类型的低4位
#else
                    cb(result, h, gcvalue(value), RelationShipType_TableValue1, NULL, ttnov(key), NULL);//
#endif
                }
            }
        }
    }
        
}

//从根节点开始, 遍历每个table和lua Closure
//1 table: 对table中的key-value进行遍历,只要有table,都执行cb
//2 lua Closure:对lua函数的所有upvalue,执行cb
void xlua_report_object_relationship(map<intptr_t, vector<RefInfo>> &result, lua_State *L, ObjectRelationshipReport cb)
{
    GCObject *p = G(L)->allgc;
    lua_Debug ar;
    int i;
    const char *name;
    while(p != NULL)
    {
        if(p->tt == LUA_TTABLE)//处理每个table
        {
            Table *h = gco2t(p);
            report_table(result, h, cb);
        }
#if LUA_VERSION_NUM >= 504
        else if(p->tt == LUA_VLCL)//处理每个lua closure中的 upvalue
#else
        else if(p->tt == LUA_TLCL)
#endif
        {
            LClosure *cl = gco2lcl(p);
            lua_lock(L);

#if LUA_VERSION_NUM >= 504
            setclLvalue(L, &(L->top->val), cl);//将栈顶top指向lua closure
#else
            setclLvalue(L, L->top, cl);
#endif
            
            api_incr_top(L);
            lua_unlock(L);

            lua_pushvalue(L, -1);

            lua_getinfo(L, ">S", &ar);//获取lua closure 的函数信息, 执行完后,pop (lua closure)

            for(i = 1; ;i++)
            {
                name = lua_getupvalue(L, -1, i);//如获取成功,将upvalue(TValue*) push到栈顶, 返回upvalue的名字
                if(name == NULL)
                    break;

                const void *pv = lua_topointer(L, -1);
                if(*name != '\0' && LUA_TTABLE == lua_type(L, -1))
                {
                    /*GCObject *p1 = G(L)->allgc;
                    while(p1 != NULL)
                    {
                        const void *p1c = p1;
                        if(pv == p1c)
                        {
                            cout << "123" << endl;
                        }
                        p1 = p1->next;
                    }*/
                    
                    cb(result, cl, pv, RelationShipType_UpVALUE5, ar.short_src, ar.linedefined, name);//lua closure中的upvalue. 传入src文件名和行号?问题,如果是dostring,那这个src是什么?{short_src为[C], linedefined为-1}
                }
                lua_pop(L, 1);//pop栈顶的 upvalue
            }
            lua_pop(L, 1);
        }
        p = p->next;
    }
}

void* xlua_registry_pointer(lua_State *L)
{
    return gcvalue(&G(L)->l_registry);
}

void* xlua_global_pointer(lua_State *L)
{
    Table *reg = hvalue(&G(L)->l_registry);
    const TValue *global;
    lua_lock(L);
    global = luaH_getint(reg, LUA_RIDX_GLOBALS);
    lua_unlock(L);
    return gcvalue(global);
}


class Data
{
public:
    int Memory = 0;
    map<intptr_t, int> TableSizes = map<intptr_t, int>();//<table的地址, table的大小>
public:
    string ToString()
    {
        string s = "memory:" + to_string(Memory) + ", table count:" + to_string(TableSizes.size());
        s.append("\n");

        if(TableSizes.size() < 10)
        {
            for(auto iter = TableSizes.begin(); iter != TableSizes.end(); ++iter)
            {
                s.append("table(" + to_string((int)iter->first) + "):" + to_string(iter->second) + "\n");
            }
        }
        else
        {
            s.append("Too much table...\n");
        }

        return s;
    }

    int PotentialLeakCount(){return TableSizes.size();}
};


void TableSizeReport_Func (Data &data, const void *p, int size)
{
    data.TableSizes[(intptr_t)p] = size;
}


string UNKNOW_KEY = "???";
string METATABLE_KEY = "__metatable";
string KEY_OF_TABLE = "!KEY!";

//使用type和几个信息, 返回一个string key
static string makeKey(RelationShipType type, const char *key, double d, const char *key2)
{
    switch (type)
    {
    case RelationShipType_TableValue1://table中的散列表部分 node[key]是一个table, 1 key为其他类型, 2 key为string
        return key == NULL? "LuaTypes(" + to_string((int)(d))+")" : key;
    case RelationShipType_NumberKeyTableValue2://1 table中的散列表部分 node[key]是一个table, key为number; 2 table中的数组部分里面存在table
        return "[" + to_string(d) + "]";
    case RelationShipType_KeyOfTable3://table中的散列表部分, node.key是一个table
        return KEY_OF_TABLE;
    case RelationShipType_Metatable4://table的元表metatable
        return KEY_OF_TABLE;
    case RelationShipType_UpVALUE5://lua closure中的upvalue, short_src, linedefined
        return "Upvalue-" + string(key) + ":local " + string(key2); 
    }

    return UNKNOW_KEY;
}

struct RefInfo
{
    string key;
    bool HasNext;//是否有父结点(表)[不包括_R和_G]
    intptr_t parent;
    bool isNumberKey;
};


intptr_t registryPointer;
intptr_t globalPointer;

//result[child].push_back(new RefInfo), 注意这里是以intptr_t(child)为key
void ObjectRelationshipReport_Func(map<intptr_t, vector<RefInfo>> &result, const void *parent, const void *child, RelationShipType type, const char * key, double d, const char *key2)
{
    if(result.find((intptr_t)child) == result.end())
    {
        result[(intptr_t)child] = vector<RefInfo>();
    }

    vector<RefInfo> &infos = result[(intptr_t)child];
    string keyOfRef = makeKey(type, key, d, key2);

    //lua closure没有父节点
    bool hasNext = type != RelationShipType_UpVALUE5;
    if(hasNext)//
    {
        if((intptr_t)parent == registryPointer)//_R
        {
            keyOfRef = "_R." + keyOfRef;
            hasNext = false;
        }
        else if((intptr_t)parent == globalPointer)//_G
        {
            keyOfRef = "_G." + keyOfRef;
            hasNext = false;
        }
    }

    RefInfo r;
    r.key = keyOfRef;
    r.HasNext = hasNext;
    r.parent = (intptr_t)parent;
    r.isNumberKey = type == RelationShipType_NumberKeyTableValue2;
    infos.push_back(r);
}

class LuaMemoryLeakChecker
{
public:
    static Data getSizeReport(lua_State *L)
    {
        Data data = Data();
        data.Memory = lua_gc(L, LUA_GCCOUNT, 0);

        //从根节点开始,遍历整个链表,如果是table, 执行函数cb
        //最终: data.TableSizes[(intptr_p)h] = table_size(h, fast)
        xlua_report_table_size(data, L, TableSizeReport_Func, 0);
        return data;
    }

    //获取一个全局的关系图
    //从根节点开始, 遍历每个table和lua closure,并记录节点之间的关系(RefInfo)
    static map<intptr_t, vector<RefInfo>> getRelationship(lua_State *L)
    {
        map<intptr_t, vector<RefInfo>> result;//<intptr_t(child), vector<RefInfo>>, 注意这里是以intptr_t(child)为key
        int top = lua_gettop(L);
        intptr_t registryPointer = (intptr_t)xlua_registry_pointer(L);
        intptr_t globalPointer = (intptr_t)xlua_global_pointer(L);

        //从根节点开始, 遍历每个table和lua closure,并记录节点之间的关系(RefInfo)
        xlua_report_object_relationship(result, L, ObjectRelationshipReport_Func);

        lua_settop(L, top);
        
        return result;
    }

    
    //返回一个新的Data, 从根节点遍历所有table, 最终
    //data.TableSizes[(intptr_p)h] = table_size(h, fast)
    static Data StartMemoryLeakCheck(lua_State *L)
    {
        lua_gc(L, 2, 0);//LUA_GCCOLLECT, performs a full garbage-collection cycle
        return getSizeReport(L);
    }

    //查找size变大的table{在from和to中都存在}
    static Data findGrowing(Data from, Data to)
    {
        Data result;
        result.Memory = to.Memory;
        for(auto iter = to.TableSizes.begin(); iter != to.TableSizes.end(); ++iter)
        {
            intptr_t key = iter->first;
            int newSize = iter->second;
            if(from.TableSizes.find(key) != from.TableSizes.end())//新增的不用管,这里只处理已有的且有增长的
            {
                int oldSize = from.TableSizes[key];
                if(oldSize < newSize)//表格大小有增长
                {
                    result.TableSizes[key] = newSize;
                }
            }
        }
        return result;
    }

    //返回有大小有增长的table信息
    static Data MemoryLeakCheck(lua_State *L, Data &last)
    {
        lua_gc(L, 2, 0);//LUA_GCCOLLECT, performs a full garbage-collection cycle
        return findGrowing(last, getSizeReport(L));
    }

    //xzxtodo
    static string MemoryLeakReport(lua_State *L, Data data, int maxLevel = 10)
    {
        lua_gc(L, 2, 0);//LUA_GCCOLLECT, performs a full garbage-collection cycle
        auto relationshipInfo = getRelationship(L);

        string sb;
        sb.append("total memory: ").append(to_string(data.Memory)).append("\n");

        for(auto iter = data.TableSizes.begin(); iter != data.TableSizes.end(); ++iter)
        {
            intptr_t key = iter->first;
            int val = iter->second;
            
            if(relationshipInfo.find(key) == relationshipInfo.end())
                continue;

            vector<RefInfo> infos = relationshipInfo[key];//会不会存在闭环的情况
            vector<RefInfo> infosNew;
            
            vector<string> paths;
            for(int  i = 0; i < maxLevel; i++)
            {
                infosNew.clear();
                
                int pathCount = paths.size();

                //hasNext == false
                for(auto iterInfo = infos.begin(); iterInfo != infos.end(); ++iterInfo)
                {
                    RefInfo info = *iterInfo;
                    if(info.HasNext == false)
                        paths.push_back(info.key);
                }

                if(paths.size() - pathCount != infos.size())//还有 rRefInfo.hasNext==true的节点
                {
                    //hasNext = true
                    for(auto iterInfo = infos.begin(); iterInfo != infos.end(); ++iterInfo)
                    {
                        RefInfo info = *iterInfo;
                        if(info.HasNext == true)//
                        {
                            if(relationshipInfo.find(info.parent) != relationshipInfo.end())
                            {
                                vector<RefInfo> infosParent = relationshipInfo[info.parent];
                                for(auto iterInfosParent= infosParent.begin(); iterInfosParent != infosParent.end(); ++iterInfosParent)
                                {
                                    RefInfo infoParent = *iterInfosParent;
                                    
                                    RefInfo newInfo;
                                    newInfo.HasNext = infoParent.HasNext;
                                    newInfo.key = infoParent.key + "." + info.key;
                                    newInfo.parent = infoParent.parent;
                                    newInfo.isNumberKey = infoParent.isNumberKey;
                                    
                                    infosNew.push_back(newInfo);
                                }
                            }
                        }
                    }

                    infos = infosNew;//infos中始终存储着还没有回溯到最顶层parent的节点
                }
                else
                {
                    break;                    
                }
            }
            
            //因为层级关系, maxLevel, infos还可能存在回溯不到最顶部的parent的节点
            for(auto iterInfo = infos.begin(); iterInfo != infos.end(); ++iterInfo)
            {
                RefInfo info = *iterInfo;
                if(info.HasNext == true)
                {
                    paths.push_back("..." + info.key);
                }
            }

            //
            sb.append("potential leak(").append(to_string(val)).append(") in {");
            for(auto iter = paths.begin(); iter != paths.end(); ++iter)
            {
                sb.append(*iter).append(", ");
            }
            sb.append("}\n");
        }

        return sb;
    }
};


void main()
{
    lua_State* L = luaL_newstate();
    assert(L != NULL);    

    luaopen_base(L);
    luaL_openlibs(L);

    registryPointer = (intptr_t)xlua_registry_pointer(L);
    globalPointer = (intptr_t)xlua_global_pointer(L);


    int iRet = luaL_dofile(L, "test1.lua");
    if (iRet)
    {
        cout << "load file test1.lua error " << iRet << endl;
        const char* pErrorMsg = lua_tostring(L, -1);
        cout << pErrorMsg << endl;
        lua_close(L);
        
        return;
    }

    iRet = luaL_dofile(L, "test2.lua");
    if (iRet)
    {
        cout << "load file test2.lua error " << iRet << endl;
        const char* pErrorMsg = lua_tostring(L, -1);
        cout << pErrorMsg << endl;
        lua_close(L);
        
        return;
    }

    iRet = luaL_dofile(L, "test3.lua");
    if (iRet)
    {
        cout << "load file test3.lua error " << iRet << endl;
        const char* pErrorMsg = lua_tostring(L, -1);
        cout << pErrorMsg << endl;
        lua_close(L);
        
        return;
    }

    iRet = luaL_dostring(L, "function fun1() print(1234) end  fun1()");
    if (iRet)
    {
        const char* pErrorMsg = lua_tostring(L, -1);
        cout << pErrorMsg << endl;
        lua_close(L);
        return;
    }

    /* 获取debug信息
    lua_getglobal(L, "myAdd");
    lua_Debug ar;
    lua_getinfo(L, ">S", &ar);
    cout << ar.short_src << "," << ar.linedefined << endl;
    */
    
    //2、C调用Lua
    /*lua_getglobal(L, "myAdd");
    lua_pushnumber(L, 10);
    lua_pushnumber(L, 20);
    iRet = lua_pcall(L, 2, 1, 0);//2个参数，1个返回值

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
        cout << "C Call Lua, result = " << fValue << endl;
    }
    */

    
    /*
    lua_pushboolean(L, true);
    lua_setglobal(L, "shutdown_fast_leak");
    */
    

    
/* 执行update函数
    lua_getglobal(L, "update");
    iRet = lua_pcall(L, 0, 0, 0);
    if (iRet)
    {
        const char* pErrorMsg = lua_tostring(L, -1);
        cout << pErrorMsg << endl;
        lua_close(L);
        return;
    }
*/
    
    
    
    


    //lua内存分析
    
    Data dataLast = LuaMemoryLeakChecker::StartMemoryLeakCheck(L);
    cout << "Start, PotentialLeakCount:" << dataLast.PotentialLeakCount() << endl;

    string s = LuaMemoryLeakChecker::MemoryLeakReport(L, dataLast); 
    cout <<  "report-----------\n" << s <<endl;
    

    int tick = 0;
    bool finished = false;
    if(finished == false) return;

    while(true)
    {
        if(!finished)
        {
            tick++;

            //调用lua update函数
            cout << "tick update, tick = " << tick << endl;
            lua_getglobal(L, "update");
            iRet = lua_pcall(L, 0, 0, 0);
            if (iRet)
            {
                const char* pErrorMsg = lua_tostring(L, -1);
                cout << pErrorMsg << endl;
                lua_close(L);
                return;
            }

            if(tick % 30 == 0)
            {
                dataLast = LuaMemoryLeakChecker::MemoryLeakCheck(L, dataLast);//返回有大小有增长的table信息
                cout << "Update, PotentialLeakCount: "<< dataLast.PotentialLeakCount() << endl;
            }

            if(tick % 180 == 0)
            {
                string s = LuaMemoryLeakChecker::MemoryLeakReport(L, dataLast); 
                cout <<  "report-----------\n" << s <<endl;

                if(tick == 180)
                {
                    //假装解决了快速内存泄露
                    lua_pushboolean(L, true);
                    lua_setglobal(L, "shutdown_fast_leak");

                    //开启一个新的泄露检测
                    dataLast = LuaMemoryLeakChecker::StartMemoryLeakCheck(L);
                }
                else
                {
                    finished = true;
                    cout << "Finished";
                }
                
            }
            
        }
        else
        {
            break;
        }
    }
    
    
    
    
    lua_close(L);
}