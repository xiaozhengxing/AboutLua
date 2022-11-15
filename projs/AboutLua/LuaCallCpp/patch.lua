function PrintMem1(p)
	n = getClsMem1(p)
	print("------------ in Lua, mem1 = "..n)
end

p = getClsPointer()

printPointer(p)
PrintMem1(p)
setClsMem1(p, 100)
PrintMem1(p)