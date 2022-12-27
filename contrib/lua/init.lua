function printf(s, ...)
  return io.write(s:format(...))
end

function fail(msg)
  print(msg)
  os.exit(false)
end

function WEXITSTATUS(status)
  return (status & 0xff00) >> 8
end

function WTERMSIG(status)
  return status & 255
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

-- wrapper table for environment variables
-- get: environ["PATH"] = "/usr/bin:/bin"
-- set: environ["PATH"]
-- delete: environ["PATH"] = nil
-- list: for k, v in pairs(environ) do print(k, v)
environ = {}
setmetatable(environ, {
    __index = function (table, key)
      return os.getenv(key)
    end,
    __newindex = function (table, key, value)
      if value == nil then
        os.unsetenv(key)
      else
        os.setenv(key, value, 1)
      end
    end,
    __pairs = function (table)
      return next, os.environ()
    end
  })

-- lookup program in directories specified by PATH variable
function which(prog)
  if prog:match('^[./]') then return prog end

  for dir in environ['PATH']:gmatch('([^:]*)') do
    path = dir .. '/' .. prog
    ok, _ = pcall(unix.access, path, unix.X_OK)
    if ok then return path end
  end
end
