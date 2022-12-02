--参考 https://github.com/yaukeywang/LuaMemorySnapshotDump


local mri = require("SnapShot")

mri.m_cConfig.m_bAllMemoryRefFileAddTime = false

-- 打印当前 Lua 虚拟机的所有内存引用快照到文件(或者某个对象的所有引用信息快照)到本地文件。
-- strSavePath - 快照保存路径，不包括文件名。
-- strExtraFileName - 添加额外的信息到文件名，可以为 "" 或者 nil。
-- nMaxRescords - 最多打印多少条记录，-1 打印所有记录。
-- strRootObjectName - 遍历的根节点对象名称，"" 或者 nil 时使用 tostring(cRootObject)
-- cRootObject - 遍历的根节点对象，默认为 nil 时使用 debug.getregistry()。
-- MemoryReferenceInfo.m_cMethods.DumpMemorySnapshot(strSavePath, strExtraFileName, nMaxRescords, strRootObjectName, cRootObject)

collectgarbage("collect")
print(mri.m_cMethods.DumpMemorySnapshot)
mri.m_cMethods.DumpMemorySnapshot("./", "1-Before", -1)

local author = 
{
	Name = "yaukeywang",
	Job = "Game Developer",
	Hobby = "Game, Travel, Gym",
	City = "Beijing",
	Country = "China",
	Ask = function (question)
	    return "My answer is for you quesiton:" .. quesiton.."."
    end
}

_G.Author = author

-- Dump memory SnapShot again
collectgarbage("collect")
mri.m_cMethods.DumpMemorySnapshot("./", "2-After", -1)


