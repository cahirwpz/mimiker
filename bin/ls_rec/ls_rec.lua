#!/bin/lua

-- List recursively files starting from directory passed as first argument.

filetype = {
  [unix.DT_FIFO] = "fifo", [unix.DT_CHR] = "chr", [unix.DT_DIR] = "dir",
  [unix.DT_BLK] = "blk", [unix.DT_REG] = "file", [unix.DT_LNK] = "link",
  [unix.DT_SOCK] = "sock"
}

function list_recursive(dir_path)
  success, maybe_fd = pcall(unix.open, dir_path, 0)
  if not success then error(maybe_fd.msg .. ": " .. dir_path) end
  dirents = unix.getdirentries(maybe_fd)
  unix.close(maybe_fd)
  for _, dirent in ipairs(dirents) do
    d_fileno, d_type, d_name = dirent.d_fileno, dirent.d_type, dirent.d_name
    sep = dir_path:sub(-1) == "/" and "" or "/"
    path = dir_path .. sep .. d_name
    printf("[%4s, ino=%d] %s\n", filetype[d_type] or "???", d_fileno, path)
    if d_type == unix.DT_DIR and d_name ~= "." and d_name ~= ".." then
      list_recursive(path)
    end
  end
end

list_recursive(arg[1] or "/")
