--
-- Example of how to use debug.lua to debug profiles in a pure lua environment.
-- It makes it easy to manually set tags, run the way processing and check the result.
--
-- To use, make a copy of this file and gitignore your copy. (If you use the name ndebug.lua,
-- it's already gitignored.)
-- 
-- You run your copy via the lua command line:
--    > cd profiles
--    > lua5.1 debug.lua
--
-- You can then modify the input tags and rerun the file to check the output.
--
-- TODO: there are a few helper methods that are implemented in C++, which are currently
-- just mocked as empty methods in LUA. Tag processing that uses these helpers will
-- not yet work correctly in this pure LUA debugging environment.


-- for better printing
local pprint = require('lib/pprint')

-- require the debug tool
local Debug = require('lib/profile_debugger')

-- load the profile we want to debug
Debug.load_profile('foot') 

-- define some input tags. they would normally by extracted from OSM data,
-- but here we can set them manually which makes debugging the profile eaiser

local way = {
  highway = 'primary',
  name = 'Magnolia Boulevard',
  ["access:forward"] = 'no'
}

-- output will go here
local result = {}

-- call the way function
Debug.process_way(way,result)

-- print input and output
pprint(way)
print("=>")
pprint(result)
print("\n")

-- report what tags where fetched, and how many times 
Debug.report_tag_fetches()
