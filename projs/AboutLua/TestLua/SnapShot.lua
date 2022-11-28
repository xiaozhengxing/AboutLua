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
    
    if "table" == strType then --处理table
        --Check table with class Name
        if rawget(cObject, "__cname") then
            if "string" == type(cObject.__cname) then
                strName = strName.."[class:"..cObject.__cname.."]"
            end
        elseif rawget(cObject, "class") then
            if "string" == type(cObject.class) then
                strName = strName.."[class:"..cObject.class.."]"
            end
        elseif rawget(cObject, "_className") then
            if "string" == type(cObject._className) then
                strName = strName.."[class:"..cObject._className.."]"
            end
        end

        -- Check if table is _G.
        if cObject == _G then
            strName = strName.."[_G]"
        end

        -- Get metatable
        local bWeakK = false
        local bWeakV = false
        local cMt = getmetatable(cObject)
        if cMt then
            -- Check mode
            local strMode = rawget(cMt, "__mode")
            if strMode then
                if string.find(strMode, "k") then
                    bWeakK = true
                end

                if string.find(strMode, "v") then
                    bWeakV = true
                end
            end
        end

        -- Add reference and name
        cRefInfoContainer[cObject] = (cRefInfoContainer[cObject] and (cRefInfoContainer[cObject] + 1)) or 1
        if cNameInfoContainer[cObject] then
            return 
        end

        --Set name
        cNameInfoContainer[cObject] = strName

        -- Dump table key and value
        for k, v in pairs(cObject) do
            -- Check key type
            local strKeyType = type(k)
            if "table" == strKeyType then
                if not bWeakK then--递归调用
                    CollectObjectReferenceInMemory(strName..".[table:key.table]", k, cDumpInfoContainer)
                end
                
                if not bWeakV then
                    CollectObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            elseif "function" == strKeyType then
                if not bWeakK then
                    CollectObjectReferenceInMemory(strName..".[talbe:key.function]", k, cDumpInfoContainer)
                end

                if not bWeakV then
                    CollectObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            elseif "thread" == strKeyType then
                if not bWeakK then
                    CollectObjectReferenceInMemory(strName..".[table:key.thread]", k, cDumpInfoContainer)
                end

                if not bWeakV then
                    CollectObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            elseif "userdata" == strKeyType then
                if not bWeakK then
                    CollectObjectReferenceInMemory(strName..".[table:key.userdata]", k,  cDumpInfoContainer)
                end

                if not bWeakV then 
                    CollectObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            else
                CollectObjectReferenceInMemory(strName.."."..k, v, cDumpInfoContainer)
            end
        end

        -- Dump metatable
        if cMt then
            CollectObjectReferenceInMemory(strName..".[metatable]", cMt, cDumpInfoContainer)
        end

    elseif "function" == strType then
        -- Get function info.
        local cDInfo = debug.getinfo(cObject, "Su")
        
        --write this info
        cRefInfoContainer[cObject] = (cRefInfoContainer[cObject] and (cRefInfoContainer[cObject] + 1)) or 1
        if cNameInfoContainer[cObject] then
            return 
        end

        -- Set name
        cNameInfoContainer[cObject] = strName.."[line:"..tostring(cDInfo.linedefined).."@file:"..cDInfo.short_src.."]"

        --Get upvalues
        local nUpsNum = cDInfo.nups
        for i = 1, nUpsNum do
            local strUpName, cUpValue = debug.getupvalue(cObject, i)
            local strUpValueType = type(cUpValue)
            if "table" == strUpValueType then
                CollectObjectReferenceInMemory(strName..".[ups:table:"..strUpName.."]", cUpValue, cDumpInfoContainer)
            elseif "function" == strUpValueType then
                CollectObjectReferenceInMemory(strName..".[ups:function:"..strUpName.."]", cUpValue, cDumpInfoContainer)
            elseif "thread" == strUpValueType then
                CollectObjectReferenceInMemory(strName..".[ups:thread:"..strUpName.."]", cUpValue, cDumpInfoContainer)
            elseif "userdata" == strUpValueType then
                CollectObjectReferenceInMemory(strName..".[ups:userdata:"..strUpName.."]", cUpValue, cDumpInfoContainer)
            end
        end

        -- Dump evnviroment table.
        local getfenv = debug.getfenv
        if getfenv then 
            local cEnv = getfenv(cObject)
            if cEnv then
                CollectObjectReferenceInMemory(strName..".[function:environment]", cEnv, cDumpInfoContainer)
            end
        end
    elseif "thread" == strType then
        --Add reference and name
        cRefInfoContainer[cObject] = (cRefInfoContainer[cObject] and (cRefInfoContainer[cObject] + 1)) or 1
        if cNameInfoContainer[cObject] then
            return 
        end

        -- Set name
        cNameInfoContainer[cObject] = strName

        -- Dump evnvironment table
        local getfenv = debug.getfenv
        if getfenv then 
            local cEnv = getfenv(cObject)
            if cEnv then
                CollectObjectReferenceInMemory(strName..".[thread:environment]", cEnv, cDumpInfoContainer)
            end
        end

        -- Dump metatable
        local cMt = getmetatable(cObject)
        if cMt then
            CollectObjectReferenceInMemory(strName..".[thread:metatable]", cMt, cDumpInfoContainer)
        end
    elseif "userdata" == strType then
        --Add reference and name
        cRefInfoContainer[cObject] = (cRefInfoContainer[cObject] and (cRefInfoContainer[cObject] + 1)) or 1
        if cNameInfoContainer[cObject] then
            return 
        end

        -- Set name
        cNameInfoContainer[cObject] = strName
        
        -- Dump evnvironment table
        local getfenv = debug.getfenv
        if getfenv then 
            local cEnv = getfenv(cObject)
            if cEnv then
                CollectObjectReferenceInMemory(strName..".[userdata:environment]", cEnv, cDumpInfoContainer)
            end
        end

        -- Dump metatable
        local cMt = getmetatable(cObject)
        if cMt then
            CollectObjectReferenceInMemory(strName..".[userdata:metatable]", cMt, cDumpInfoContainer)
        end
    elseif "string" == strType then
        --Add reference and name
        cRefInfoContainer[cObject] = (cRefInfoContainer[cObject] and (cRefInfoContainer[cObject] + 1)) or 1
        if cNameInfoContainer[cObject] then
            return 
        end

        --set Name
        cNameInfoContainer[cObject] = strName.."["..strType.."]"
    else
        --For "number" and "boolean".
        --Add reference and name
        --[[
        cRefInfoContainer[cObject] = (cRefInfoContainer[cObject] and (cRefInfoContainer[cObject] + 1)) or 1
        if cNameInfoContainer[cObject] then
            return 
        end

        --set Name
        cNameInfoContainer[cObject] = strName.."["..strType..":"..tostring(cObject).."]"
        --]]
    end
    


end

-- Collect memory reference info of a single object from a root table or function.
-- strName - The root object name that start to search, can not be nil 
-- cObject - The root object that start to search, can not be nil 
-- cDumpInfoContainer - The container of the dump rsult info
local function CollectSingleObjectReferenceInMemory(strName, cObject, cDumpInfoContainer)
    if not cObject then
        return
    end

    if not strName then
        strName = ""
    end

    --Check container
    if not cDumpInfoContainer then
        cDumpInfoContainer = CreateObjectReferenceInfoContainer()
    end

    -- Check stack
    if cDumpInfoContainer.m_nStackLevel > 0 then
        local cStackInfo = debug.getinfo(cDumpInfoContainer.m_nStackLevel, "Sl")
        if cStackInfo then
            cDumpInfoContainer.m_strShortSrc = cStackInfo.short_src
            cDumpInfoContainer.m_nCurrentLine = cStackInfo.currentline
        end

        cDumpInfoContainer.m_nStackLevel = -1
    end

    local cExistTag = cDumpInfoContainer.m_cObjectExistTag
    local cNameAllAlias = cDumpInfoContainer.m_cObjectAliasName
    local cAccessTag = cDumpInfoContainer.m_cObjectAcessTag

    local strType = type(cObject)
    if "table" == strType then
        --Check table with class name
        if rawget(cObject, "__cname") then
            if "string" == type(cObject.__cname) then
                strName = strName.."[class"..cObject.__cname.."]"
            end
        elseif rawget(cObject, "class") then
            if "string" == type(cObject.class) then
                strName = strName.."[class:"..cObject.class.."]"
            end
        elseif rawget(cObject, "_className") then
            if "string" == type(cObject._className) then
                strName = strName .. "[class:" ..cObject._className .. "]"
            end
        end

        -- Check if table is _G.
        if cObject == _G then
            strName = strName.."[_G]"
        end

        -- Get metatable.
        local bWeakK = false
        local bWeakV = false
        local cMt = getmetatable(cObject)
        if cMt then
            -- Check mode.
            local strMode = rawget(cMt, "__mode")
            if strMode then
                if "k" == strMode then
                    bWeakK = true
                elseif "v" == strMode then
                    bWeakV = true
                elseif "kv" == strMode then
                    bWeakK = true
                    bWeakV = true
                end
            end
        end

        --Check if the specified object
        if cExistTag[cObject] and (not cNameAllAlias[strName]) then
            cNameAllAlias[strName] = true
        end

        --Add reference and name
        if cAccessTag[cObject] then
            return
        end

        -- Get this name
        cAccessTag[cObject] = true

        --Dump table key and value
        for k, v in  pairs(cObject) do
            --Check key type
            local strKeyType = type(k)
            if "table" == strKeyType then
                if not bWeakK then
                    CollectSingleObjectReferenceInMemory(strName..".[table:key.table]", k, cDumpInfoContainer)
                end

                if not bWeakV then
                    CollectSingleObjectReferenceInMemory(strName..".[table.value]", v, cDumpInfoContainer)
                end
            elseif "function" == strKeyType then
                if not bWeakK then
                    CollectSingleObjectReferenceInMemory(strName..".[table:key.function]", k, cDumpInfoContainer)
                end

                if not bWeakV then
                    CollectSingleObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            elseif "thread" == strKeyType then
                if not bWeakK then
                    CollectSingleObjectReferenceInMemory(strName..".[table:key.thread]", k, cDumpInfoContainer)
                end

                if not bWeakV then
                    CollectSingleObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            elseif "userdata" == strKeyType then
                if not bWeakK then
                    CollectSingleObjectReferenceInMemory(strName..".[table:key.userdata]", k, cDumpInfoContainer)
                end

                if not bWeakV then
                    CollectSingleObjectReferenceInMemory(strName..".[table:value]", v, cDumpInfoContainer)
                end
            else
                CollectSingleObjectReferenceInMemory(strName.."."..k, v, cDumpInfoContainer)
            end
        end

        --Dump metatable
        if cMt then
            CollectSingleObjectReferenceInMemory(strName..".[metatable]", cMt, cDumpInfoContainer)
        end
    elseif "function" == strType then
        --xzxtodo
    end
    



    --xzxtodo
end







