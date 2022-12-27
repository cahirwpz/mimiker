internal = {}

function internal.cat(args)
  for i = 2, #args do
    for line in io.lines(args[i]) do
      print(line)
    end
  end
end

