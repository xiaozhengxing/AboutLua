--参考 https://github.com/yaukeywang/LuaMemorySnapshotDump


local cConfig = 
{
    m_bAllMemoryRefFileAddTime = true,--保存的文件名加上时间戳
    m_bSingleMemoryRefFileAddTime = true,
    m_bCompareMemoryRefFileAddTime = true,
}

local function FormatDateTimeNow()
    local cDateTime = os.date("*t")
    local strDateTime = string.format("%04d%02d%02d-%02d%02d%02d", tostring(cDateTime.year), tostring(cDateTime.month), tostring(cDateTime.day),
		tostring(cDateTime.hour), tostring(cDateTime.min), tostring(cDateTime.sec))
	return strDateTime
end

print(FormatDateTimeNow())

--使用最原始的tostring(cObject),而不触发其元表
local function GetOriginalToStringResult(cObject)
    if not cObject then
        return ""
    end

    local cMt = getmetatable(cObject)
    if not cMt then
        return tostring(cObject)
    end

    --检测元表中的tostring方法
    local cToString = rawget(cMt, "__tostring")
	if cToString then
		rawset(cMt, "__tostring", nil)--先设置为nil
		strName = tostring(cObject)
		rawset(cMt, "__tostring", cToString)--恢复
	else
		strName = tostring(cObject)
	end

	return strName
end