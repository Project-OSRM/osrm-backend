-- lua tag test server, communicating via tcp port 7878.
-- it can speed up the execution of large number of tag tests,
-- because you don't have to launch the lua interpreter for each test.
-- instead you launch the tag test server and keep it running.
-- for each test you send it some tags which are processed by a
-- profile. the values computed by the profile are returned and can
-- be used to validate against expected values.

-- the general flow of events is:
--
-- 1. it receives a set of osm tags, encoded as json
-- 2. it uses the specified profile to process the tags
-- 3. it returns the resulting values encoded as json
--
-- example usage:
--
-- first start the test server:
--   > cd profiles
--   > lua lib/test_server.lua
--   Starting tag test server on port 7878
--
-- then use eg. netcat to send tags and get back values:
--   > nc 127.0.0.1 7878
--   {"highway":"primary"}
--   {
--       "forward_speed" : 36,
--       "duration" : 0,
--       "road_classification" : {
--           },
--       "forward_mode" : 1,
--       "backward_mode" : 1,
--       "backward_speed" : 36
--   }
--
-- for use in eg. cucumber testing, the cucumber support code would interface
-- with the test server instead of netcat.



local Debug = require('lib/profile_debugger')
local pprint = require('lib/pprint')
json = require 'lib/json'





-- load profile
Debug.load_profile('testbot')




-- load namespace
local socket = require("socket")

-- create a TCP socket and bind it to the local host, at any port
local server = assert(socket.bind("*", 7878))

-- find out which port the OS chose for us
local ip, port = server:getsockname()

-- print a message informing what's up
print("Starting tag test server on port " .. port)



-- wait for a connection from any client
local client = server:accept()

-- make sure we don't block waiting for this client's line
--client:settimeout(10)

-- loop forever waiting for clients
while 1 do
  -- receive the line
  local line, err = client:receive()
  -- if there was no error, send it back to the client
  if not err then
  	local way = json.parse(line)
		local result = {}
		Debug.way_function(way,result)
  	client:send(json.encode(result) .. "\n")
  else
  	break
 	end
end

print("Starting tag test server on port " .. port)

  -- done with client, close the object
  --client:close()

