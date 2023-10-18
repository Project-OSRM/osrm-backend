--
-- Fetch a way from the OpenStreetMap API and run the given profile over it.
--
-- You'll need to install xml2lua first (may require admin privileges):
--    > luarocks install xml2lua
--
-- You may also need to install luasec and luasocket if you don't have them already.
--
-- Then to test way 2606296 using the foot profile:
--    > lua debug_way.lua foot 2606296
--

-- initialise libraries
local pprint = require('lib/pprint')
local Debug = require('lib/profile_debugger')
local xml2lua = require('xml2lua')
local handler = require('xmlhandler.tree')
local https = require('ssl.https')

-- load the profile
Debug.load_profile(arg[1])

-- load way from the OSM API
local url = 'https://api.openstreetmap.org/api/0.6/way/'..arg[2]
local body, statusCode, headers, statusText = https.request(url)

-- parse way tags
local parser = xml2lua.parser(handler)
parser:parse(body)

-- convert XML-flavoured table to a simple k/v table
local way = {}
for i, p in pairs(handler.root.osm.way.tag) do
  way[p._attr.k] = p._attr.v
end

-- call the way function
local result = {}
Debug.process_way(way,result)

-- print input and output
pprint(way)
print("=>")
pprint(result)
