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


--Create a container to collect the mem ref info results from a dumped file
--strObjectName - The object name you need to collect info.
--cObject - The object you need to collect info
local function CreateSingleObjectReferenceInfoContainer(strObjectName, cObject)
    local cContainer = {}

    --Contain[address]-[true] info
    local cObjectExistTag = {}
    setmetatable(cObjectExistTag, {__mode = "k"})

    --Contain [name] - [true] info
    local cObjectAliasName = {}

    --Contain [access] - [true] info
    local cObjectAccessTag = {}
    setmetatable(cObjectAccessTag, {__mode = "k"})

    --Set members
    cContainer.m_cObjectExistTag = cObjectExistTag
    cContainer.m_cObjectAliasName = cObjectAliasName
    cContainer.m_cObjectAcessTag = cObjectAccessTag

    -- For stack info
    cContainer.m_nStackLevel = -1
    cContainer.m_strShortSrc = "None"
    cContainer.m_nCurrentLine = -1

    --Init with object values
    cContainer.m_strObjectName = strObjectName
    cContainer.m_strAddressName = (("string" == type(cObject)) and ("\"" .. tostring(cObject) .. "\"")) or GetOriginalToStringResult(cObject)
    cContainer.m_cObjectExistTag[cObject] = true
    
    return cContainer
end


-- 这个函数比较重要
-- Collect memory reference info from a root table or function
-- strName - The root object name that start to search, default is "_G" if leave this to nil 
-- cObject - The root object that start to search, default is _G if leave this to nil 
-- cDumpInfoContainer - The container of the dump result info
local function CollectObjectReferenceInMemory(strName, cObject, cDumpInfoContainer)
    if not cObject then
        return
    end

    if not strName then
        strName = ""
    end

    -- Check Container
    if (not cDumpInfoContainer) then
        cDumpInfoContainer = CreateObjectReferenceInfoContainer()
    end

    -- check stack
    if cDumpInfoContainer.m_nStackLevel > 0 then
        local cStackInfo = debug.getinfo(cDumpInfoContainer.m_nStackLevel, "Sl")
        if cStackInfo then
            cDumpInfoContainer.m_strShortSrc = cStackInfo.short_src
            cDumpInfoContainer.m_nCurrentLine = cStackInfo.currentline
        end

        cDumpInfoContainer.m_nStackLevel = -1
    end

    -- Get ref and name info
    local cRefInfoContainer = cDumpInfoContainer.m_cObjectReferenceCount
    local cNameInfoContainer = cDumpInfoContainer.m_cObjectAdressToName

    local strType = type(cObject)
    if "table" == strType then
        --Check table with class Name
        if rawget(cObject, "__cname") then
            if "string" = type(cObject.__cname)
                strName = strName.."[class:"..cObject.__cname.."]"
            end
        elseif rawget(cObject, "class") then
            if "string" == type(cObject.class) then
                strName = strName.."[class:"..cObject.class.."]"
            end
        else if rawget--xzxtodo
        end
    end
    


end







