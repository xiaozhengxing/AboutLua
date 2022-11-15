p = getClsPointer()

pUData = makeClsUserData(p, "Cls")

pUData:printMem1()

print("in Lua, pUData.mem1 = "..pUData.mem1)

pUData.mem1 = 456
pUData:printMem1()

pUData:setMem1(789)
print(pUData.mem1)
