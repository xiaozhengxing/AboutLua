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



 

