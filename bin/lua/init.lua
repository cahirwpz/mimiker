function printf(s, ...)
  return io.write(s:format(...))
end

--[[
opts, args = getopt(arg, "a:vpb:h")
for k, v in pairs(opts) do print("opt['" .. k .. "'] -> " .. tostring(v)) end
for i = 1,#args do print("arg[" .. i .. "] -> " .. args[i]) end
--]]
function getopt(arg, optstr)
  -- parse options
  li, opts = 1, {}
  while li <= #arg do
    if arg[li] == "--" then li = li + 1; break end
    if not arg[li]:match("^%-.") then break end
    for oi = 2, #arg[li] do
      opt = arg[li]:sub(oi, oi)
      optdsc = optstr:match(opt .. ":?")
      if not optdsc then error("unknown option -- " .. opt) end
      if #optdsc == 1 then
        opts[opt] = true
      else
        li = li + 1
        if li > #arg then error("option requires an argument -- " .. opt) end
        opts[opt] = arg[li]
      end
    end
    li = li + 1
  end

  -- copy trailing arguments
  args = {}
  for i = li, #arg do
    args[#args + 1] = arg[i]
  end

  return opts, args
end
