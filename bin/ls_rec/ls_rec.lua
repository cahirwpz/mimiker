#!/bin/lua

-- List recursively files starting from directory passed as first argument.

filetype = {
  [unix.DT_FIFO] = "fifo", [unix.DT_CHR] = "chr", [unix.DT_DIR] = "dir",
  [unix.DT_BLK] = "blk", [unix.DT_REG] = "file", [unix.DT_LNK] = "link",
  [unix.DT_SOCK] = "sock"
}

function list(opts, dir_path)
  success, maybe_fd = pcall(unix.open, dir_path, 0)
  if not success then print(maybe_fd.msg .. ": " .. dir_path); return end
  dirents = unix.getdirentries(maybe_fd)
  unix.close(maybe_fd)
  for _, dirent in ipairs(dirents) do
    d_fileno, d_type, d_name = dirent.d_fileno, dirent.d_type, dirent.d_name
    sep = dir_path:sub(-1) == "/" and "" or "/"
    path = dir_path .. sep .. d_name
    printf("[%4s, ino=%d] %s%s\n", filetype[d_type] or "???", d_fileno, path,
           (opts.p and d_type == unix.DT_DIR) and "/" or "")
    if d_type == unix.DT_DIR and d_name ~= "." and d_name ~= ".." then
      if opts.R then list(opts, path) end
    end
  end
end

usage = [[
  -p  Write a '/' after each filename that is a directory.
  -R  Recursively list directories.
]]

ok, opts, args = pcall(getopt, arg, "Rp")
if not ok then
  print(arg[0] .. ": " .. opts)
  print("Usage: " .. arg[0] .. " [-Rp] [file1 file2 ...]\n")
  print(usage)
  os.exit(false)
end

if #args == 0 then args[1] = "/" end

for i = 1, #args do
  list(opts, args[i])
end
