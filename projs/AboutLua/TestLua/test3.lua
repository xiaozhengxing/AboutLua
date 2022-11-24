
TestTT = {
    ["AudioMgr"] = {
        ["RegisterEvent"] = {
            0
        },
        ["ToBytes"] = {
                    20562,20563
        },
        ["ToBytes_Byte_L0...,0..._R"] = {
            20562
        },
        ["ToBytes_Byte[]"] = {
            20563
        },
    }
}

TestTable88 = {
    ["key1"]=666,
    ["key2"]=777
}

local TestTable99 = {
    1,2,3,4,5,6,7,8,9,10
}
table.insert(TestTable99, 1)

--function HandleTestTable99()
  --  table.insert(TestTable99, 1)
--end

shutdown_fast_leak = false

function myAdd(a, b)
    if shutdown_fast_leak  then
        return a + b + 100
    end
    return a + b + 1
end

function update()
    if not shutdown_fast_leak then
        make_leak1()
        make_leak2()
    end
    
    innocent()
    slow_leak()
end



 

