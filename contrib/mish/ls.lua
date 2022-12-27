ls = {}

-- List files in directories passed in positional arguments.

ls.filetype = {
  [unix.DT_FIFO] = "fifo", [unix.DT_CHR] = "chr", [unix.DT_DIR] = "dir",
  [unix.DT_BLK] = "blk", [unix.DT_REG] = "file", [unix.DT_LNK] = "link",
  [unix.DT_SOCK] = "sock"
}

function ls.list(dir_path)
  success, maybe_fd = pcall(unix.open, dir_path, 0)
  if not success then print(maybe_fd.msg .. ": " .. dir_path); return end
  dirents = unix.getdents(maybe_fd)
  unix.close(maybe_fd)
  for _, dirent in ipairs(dirents) do
    d_fileno, d_type, d_name = dirent.d_fileno, dirent.d_type, dirent.d_name
    sep = dir_path:sub(-1) == "/" and "" or "/"
    path = dir_path .. sep .. d_name
    printf("[%4s, ino=%d] %s%s\n", ls.filetype[d_type] or "???", d_fileno, path,
           (arg.p and d_type == unix.DT_DIR) and "/" or "")
    if d_type == unix.DT_DIR and d_name ~= "." and d_name ~= ".." then
      if arg.R then ls.list(path) end
    end
  end
end

ls.usage = [[
  -p  Write a '/' after each filename that is a directory.
  -R  Recursively list directories.
]]

function internal.ls(argv)
  arg = table.move(argv, 1, #argv, 0, {})

  ok, err = pcall(getopt, arg, "Rp")
  if not ok then
    print(arg[0] .. ": " .. err)
    print("Usage: " .. arg[0] .. " [-Rp] [file1 file2 ...]\n")
    print(ls.usage)
    os.exit(false)
  end

  if #arg == 0 then arg[1] = unix.getcwd() end

  for i = 1, #arg do
    ok, err = pcall(ls.list, arg[i])
    if not ok then
      print(arg[i] .. ": " .. err.msg)
    end
  end
end
