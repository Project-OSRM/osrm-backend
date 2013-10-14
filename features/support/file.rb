class File

  #read last n lines of a file. useful for getting last part of a big log file.
  def tail(n)
    buffer = 1024
    idx = (size - buffer).abs
    chunks = []
    lines = 0

    begin
      seek(idx)
      chunk = read(buffer)
      lines += chunk.count("\n")
      chunks.unshift chunk
      idx -= buffer
    end while lines < ( n + 1 ) && pos != 0

    tail_of_file = chunks.join('')
    ary = tail_of_file.split(/\n/)
    lines_to_return = ary[ ary.size - n, ary.size - 1 ]
  rescue
    ["Cannot read log file!"]
  end
end