-- Enable calling our lua profile code directly from the lua command line,
-- which makes it easier to debug.
-- We simulate the normal C++ environment by defining the required globals and functions.

-- See debug_example.lua for an example of how to require and use this file.

-- for more convenient printing of tables
local pprint = require('lib/pprint')


-- globals that are normally set from C++

-- should match values defined in include/extractor/road_classification.hpp
road_priority_class = {
  motorway = 0,
  trunk = 2,
  primary = 4,
  secondary = 6,
  tertiary = 8,
  main_residential = 10,
  side_residential = 11,
  link_road = 14,
  bike_path = 16,
  foot_path = 18,
  connectivity = 31,
}

-- should match values defined in include/extractor/travel_mode.hpp
mode = {
  inaccessible = 0,
  driving = 1,
  cycling = 2,
  walking = 3,
  ferry = 4,
  train = 5,
  pushing_bike = 6,
}

-- Mock C++ helper functions which are called from LUA.
-- TODO
-- Debugging LUA code that uses these will not work correctly
-- unless we reimplement the methods in LUA.

function durationIsValid(str)
  return true
end

function parseDuration(str)
  return 1
end

function canonicalizeStringList(str)
  return str
end

 
 
-- debug helper
local Debug = {}
 
-- helpers for sorting associative array
function Debug.get_keys_sorted_by_value(tbl, sortFunction)
  local keys = {}
  for key in pairs(tbl) do
    table.insert(keys, key)
  end

  table.sort(keys, function(a, b)
    return sortFunction(tbl[a], tbl[b])
  end)

  return keys
end

-- helper for printing sorted array
function Debug.print_sorted(sorted,associative)
  for _, key in ipairs(sorted) do
    print(associative[key], key)
  end
end

function Debug.report_tag_fetches()
  print("Tag fetches:")
  sorted_counts = Debug.get_keys_sorted_by_value(Debug.tags.counts, function(a, b) return a > b end)
  Debug.print_sorted(sorted_counts, Debug.tags.counts)
  print(Debug.tags.total, 'total')
end

function Debug.load_profile(profile)
  Debug.functions = require(profile)
  Debug.profile = Debug.functions.setup()
end

function Debug.reset_tag_fetch_counts()
  Debug.tags = {
    total = 0,
    counts = {}
  }
end

function Debug.register_tag_fetch(k)
  if Debug.tags.total then
    Debug.tags.total = Debug.tags.total + 1
  else
    Debug['tags']['total'] = 1
  end

  if Debug['tags']['counts'][k] then
    Debug['tags']['counts'][k] = Debug['tags']['counts'][k] + 1
  else
    Debug['tags']['counts'][k] = 1
  end

end

function Debug.process_way(way,result)
  
  -- setup result table
  result.road_classification = {}
  result.forward_speed = -1
  result.backward_speed = -1
  result.duration = 0
  result.forward_classes = {}
  result.backward_classes = {}
  
  -- intercept tag functions normally provided via C++
  function way:get_value_by_key(k)
    Debug.register_tag_fetch(k)
    return self[k]
  end
  function way:get_location_tag(k)
    return nil
  end
  
  -- reset tag counts
  Debug:reset_tag_fetch_counts()
  
   -- call the way processsing function
  Debug.functions.process_way(Debug.profile,way,result)
end

return Debug
