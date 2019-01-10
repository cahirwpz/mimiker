function printf(s, ...)
  return io.write(s:format(...))
end

--[[
getopt(arg, "a:vpb:h")
for k, v in pairs(arg) do
  print("arg[" .. tostring(k) .. "] -> " .. tostring(v))
end
--]]
function getopt(arg, optstr)
  -- parse options
  li = 1
  while li <= #arg do
    if arg[li] == "--" then li = li + 1; break end
    if not arg[li]:match("^%-.") then break end
    for oi = 2, #arg[li] do
      opt = arg[li]:sub(oi, oi)
      optdsc = optstr:match(opt .. ":?")
      if not optdsc then error("unknown option -- " .. opt, 0) end
      if #optdsc == 1 then
        arg[opt] = true
      else
        li = li + 1
        if li > #arg then error("option requires an argument -- " .. opt, 0) end
        arg[opt] = arg[li]
      end
    end
    li = li + 1
  end

  -- move trailing arguments to the beginning of the table
  j = 1
  for i = li, #arg do
    arg[j], arg[i] = arg[i], nil
    j = j + 1
  end
  for i = j, #arg do arg[i] = nil end
end
