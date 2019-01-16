#!/bin/lua

builtin = {}

function builtin.exit(args)
  os.exit(tonumber(args[2]))
end

internal = {}

function internal.cat(args)
  for i = 2, #args do
    for line in io.lines(args[i]) do
      print(line)
    end
  end
end

-- spawn a process
function spawn(prog, argv)
  pid = unix.fork()
  if pid then
    _, status = unix.waitpid(pid, 0)
    signo = WTERMSIG(status)
    code = WEXITSTATUS(status)
    if signo > 0 then
      print(argv[1] .. ": terminated by " .. signo .. " signal!")
    elseif code > 0 then
      print(argv[1] .. ": exited with " .. code .. " code!")
    end
  else
    if type(prog) == "function" then
      os.exit(prog(argv))
    else
      argv[1] = prog
      err = unix.execve(prog, argv)
      fail(prog .. ": " .. err.msg)
    end
  end
end

function execute(argv)
  -- lookup builtin command
  cmd = builtin[argv[1]]
  if cmd then return cmd(argv) end

  -- is that an internal command ?
  prog = internal[argv[1]]
  if prog then return spawn(prog, argv) end

  -- is that an external command ?
  prog = which(argv[1])
  if prog then return spawn(prog, argv) end

  print(argv[1] .. ": No such command")
end

function mish()
  while true do
    -- print prompt
    io.stdout:write("# ")
    io.stdout:flush()

    -- read line and split it into words
    line = io.stdin:read("l")
    if not line then break end
    argv = {}
    for word in line:gmatch("%g+") do
      table.insert(argv, word)
    end

    if #argv then pcall(execute, argv) end
  end
end

-- setup default environment variables
if not environ["PATH"] then
  environ["PATH"] = "/usr/bin:/bin"
end

mish()
