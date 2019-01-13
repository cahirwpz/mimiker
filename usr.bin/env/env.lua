#!/bin/lua

-- Call other program with environment variables set.
-- Example use:
--   /usr/bin/env HOME=/root PATH=/bin:/usr/bin env

function fail(msg)
  print(msg)
  os.exit(false)
end

-- print environment variables
function printenv()
  for k, v in pairs(environ) do
    print(k .. "=" ..  v)
  end
end

-- lookup specified program in directories specified by PATH variable
function lookup(prog)
  for dir in environ['PATH']:gmatch('([^:]*)') do
    path = dir .. '/' .. prog
    ok, _ = pcall(unix.access, path, unix.X_OK)
    if ok then return path end
  end
end

-- main program begins here
if #arg == 0 then printenv(); os.exit() end

-- set environment variables from command line
i = 1
while i <= #arg do
  k, v = arg[i]:match("^([^=]*)=(.*)$")
  if not k then break end
  environ[k] = v
  i = i + 1
end

-- move program parameters to another table
argv = {}
table.move(arg, i, #arg, 1, argv)

-- find program to launch
if not argv[1]:match('^[./]') then
  prog = lookup(argv[1])
  if not prog then fail(argv[1] .. ": No such file") end
  argv[1] = prog
end

-- ... and finally launch it
err = unix.execve(argv[1], argv)
fail(argv[1] .. ": " .. err.msg)
