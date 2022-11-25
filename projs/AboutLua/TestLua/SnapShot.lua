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

