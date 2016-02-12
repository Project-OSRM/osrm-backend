-- Parse duration strings

local Duration = {}

function Duration.is_valid(str)
end  

-- parse most common formats described here:
-- http://wiki.openstreetmap.org/wiki/Key:duration

function Duration.parse(str)
  if not str then
    return
  end
  
  local min,hour,sec
  
  -- hh:mm:ss
  hour,min,sec = str:match("^%s*(%d+):(%d+):(%d+)")
  if hour and min and sec then
    return tonumber(hour)*60*60 + tonumber(min)*60 + tonumber(sec)
  end
  
  -- hh:mm
  hour,min = str:match("^%s*(%d+):(%d+)")
  if hour and min then
    return tonumber(hour)*60*60 + tonumber(min)*60
  end
  
  -- ss
  min = str:match("^%s*(%d+)")
  if min then
    return tonumber(min)*60
  end


  -- PT... with D=days, H=hours, M=minutes, S=seconds
  if str:match("^%s*PT") then
    -- PThh:mm:ss
    
    hour,min,sec = str:match("^%s*PT(%d+):(%d+):(%d+)")
    if hour and min and sec then
      return tonumber(hour)*60*60 + tonumber(min)*60 + tonumber(sec)
    end

    day = str:match("(%d+)D")
    hour = str:match("(%d+)H")
    min = str:match("(%d+)M")
    sec = str:match("(%d+)S")
    
    local total = 0
    if day then
      total = total + tonumber(day)*24*60*60
    end
    if hour then
      total = total + tonumber(hour)*60*60
    end
    if min then
      total = total + tonumber(min)*60
    end
    if sec then
      total = total + tonumber(sec)
    end
    return total
  end



end

return Duration
