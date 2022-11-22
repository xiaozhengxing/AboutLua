
local local_leak = {}
global_leak = {a={}}--global_leak.a.b = global_leak

local no_leak = {}

function make_leak1()
    table.insert(local_leak, 1)
    table.insert(global_leak, {})
end

--会不断创建并持有新table, 但其实没泄露
function innocent()
    no_leak.a = {x=1}
    no_leak.b = {y=1}
end



