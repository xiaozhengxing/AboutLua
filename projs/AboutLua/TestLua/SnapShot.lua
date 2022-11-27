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

local function CreateObjectReferenceInfoContainer()
    --Create new container
    local cContainer = {}

    -- Contain [table/function] - [reference count] info
    local cObjectReferenceCount = {}
    setmetatable(cObjectReferenceCount, {__mode="k"})

    -- Contain [table/function] - [name] info
    local cObjectAddressToName = {}
    setmetatable(cObjectAddressToName, {__mode="k"})

    --set members
    cContainer.m_cObjectReferenceCount = cObjectReferenceCount
    cContainer.m_cObjectAdressToName = cObjectAddressToName

    --For Stack info
    cContainer.m_nStackLevel = -1
    cContainer.m_strShortSrc = "None"
    cContainer.m_nCurrentLine = -1


    return cContainer
end

local function CreateObjectReferenceInfoContainerFromFile(strFilePath)
    local cContainer = CreateObjectReferenceInfoContainer()
    cContainer.m_strShortSrc = strFilePath

    --Cache ref info
    local cRefInfo = cContainer.m_cObjectReferenceCount
    local cNameInfo = cContainer.m_cObjectAdressToName

    --Read each line from file.
    local cFile = assert(io.open(strFilePath, "rb"))
    for strLine in cFile:lines() do 
        local strHeader = string.sub(strLine, 1, 2)--返回前两个字符
        if "--" ~= strHeader then
            local _,_, strAddr, strName, strRefCount = string.find(strLine, "(.+)\t(.*)\t(%d+)")
            if strAddr then
                cRefInfo[strAddr] = strRefCount
                cNameInfo[strAddr] = strName
            end
        end
    end

    -- Close and clear file handler
    io.close(cFile)
    cFile = nil 

    return  cContainer
end


