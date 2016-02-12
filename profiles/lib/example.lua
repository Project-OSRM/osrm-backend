-- Shows how to run/try the processing manually from LUA.
-- To run this, cd to the profiles/ folder, then run:
--
-- > lua5.1 lib/test.lua
--
-- (You might need to replace lua5.1 with your LUA version)

local Mode = require("lib/bicycle")
local pprint = require('lib/pprint')

tags = Tags:new( {
  --route ='ferry',
  --railway='train',
  --duration = '0:01',
  name = 'Avenue',
  highway='primary',
  --building='yes',
  --junction='roundabout',
  ['bicycle:forward'] = 'no',
} )

result = {}
Mode:process(tags,result)

pprint('Input tags:')
pprint(tags)

print('\n=>\n')

print('Output Common:')
pprint( Mode.tmp.main.common )

print('Output forward:')
pprint( Mode.tmp.main.forward )

print('Output backward:')
pprint( Mode.tmp.main.backward )
