
local another_leak = {a={{b={}}}}

function make_leak2()
    table.insert(another_leak.a[1].b, 1)
end

local t = 1

slow_global_leak = {}

debug.getregistry()['ref_another_leak'] = another_leak

function slow_leak()
    if t == 40 then
        t = 0
        table.insert(slow_global_leak, {x=0, y=1})
    else
        t = t + 1
    end   
end

