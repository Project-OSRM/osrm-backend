-- Base class for profile processers.
--
-- Note that LUA has no real class support, but we can emulate them.

local Tags = require('lib/tags')
local Set = require('lib/set')
local Sequence = require('lib/sequence')
local Table = require('lib/table')
local Convert = require('lib/convert')
local Duration = require('lib/duration')
local Access = require("lib/access")


-- Base class and simply class mechanics
Base = {}

function Base:subClass(settings)
  object = self:new()
  return object
end

function Base:new(object)
  object = object or {}
  setmetatable(object, self)
  self.__index = self         -- trick to save memory, see http://lua-users.org/lists/lua-l/2013-04/msg00617.html
  return object
end

-- All profiles use the same set of mode codes
Base.modes = {
  none = 0,
  generic = 1,
  car = 2,
  bike = 3,
  foot = 4,
  ferry = 5,
  train = 6,
  movable_bridge = 7,
}

-- Each subclasses has it's own settings copy and can overwrite/modify them as needed.
-- A handy way of doing this is by using the utility method to deep merge tables.
Base.settings = {
  name = 'base',
  mode = Base.modes.generic,
  oneway_handling = true,
  maxspeed_reduce = true,
  maxspeed_increase = true,
  use_fallback_names = true,
  ignore_areas = false,
  ignore_buildings = true,
  access = {
    tags = Sequence { 'access' },
    whitelist = Set { 'yes', 'permissive', 'designated', 'destination' },
    blacklist = Set { 'no', 'private', 'agricultural', 'forestry', 'emergency', 'psv' },
    restricted = Set { 'destination', 'delivery' },
  },
  barrier_whitelist = Set { 'entrance', 'no' },
  way_speeds = {},
  accessible = {},
  routes = {
    route = {
      ferry = { speed = 5, mode = Base.modes.ferry },
      shuttle_train = { speed = 20, mode = Base.modes.train }
    },
    bridge = {
      movable = { speed = 5, mode = Base.modes.movable_bridge },
    },
    railway = {
        train = { speed = 50, mode = Base.modes.train, require_access=true },
    },
  },
  nonexisting = Set { 'construction', 'proposed' },
} 

-- Tag helper functions

function Base:tag(key)
  return self.tags:get_value_by_key(key)
end

function Base.present(str)
  return str ~= nil and str ~= ''
end

function Base.empty(str)
  return str == nil or str == ''
end

function Base.extract_int(tag)
  if tag then
    return tonumber(tag:match("%d*"))
  end
end

function Base.is_tag_in_table(tag,table)
  return Base.present(tag) and table and table[tag]
end

function Base.tag_lookup(key,table)
  local tag = self:tag(key)
  return Base.present(tag) and table and table[tag]
end

function Base:is_nonexisting(tag)
  return Base.present(tag) and self.settings.nonexisting[tag]
end


-- Main process method entry point

function Base:process(tags,result)
  self:prepare_processing(tags,result)
  if self:pre_process_combined() == false then
    return
  end
  self:process_each_direction(tags,result)
  self:post_process_combined()
  self:output(result)
end

-- Process each direction

function Base:process_each_direction(tags,result)
  self:process_direction(self.tmp.main.forward)
  self:process_direction(self.tmp.main.backward)
end

-- Prepare processsing

function Base:prepare_processing(tags,result)
  self.tags = tags
  self.tmp = {
    main = {
      common = {},
      forward = {
--      _name = self.settings.name .. '/' .. 'forward',
      direction = 'forward',
      debug = {}
      },
      backward = {
--        _name = self.settings.name .. '/' .. 'backward',
        direction = 'backward',
        debug = {}
      }
    }
  }
  self.result = {}
end

-- Copy result to the structure that the C++ code expects
-- (Could be avoided by modifying the C++ side.)

function Base:output(result)
 if self.tmp.main.common.name then result.name = self.tmp.main.common.name end
 if self.tmp.main.common.roundabout then result.roundabout = self.tmp.main.common.roundabout end
 if self.tmp.main.common.startpoint then result.is_startpoint = self.tmp.main.common.startpoint end
 if self.tmp.main.common.duration then result.duration = self.tmp.main.common.duration end
 
 if self.tmp.main.forward.mode then
   result.forward_mode = self.tmp.main.forward.mode
   if self.tmp.main.forward.speed then result.forward_speed = self.tmp.main.forward.speed end
   if self.tmp.main.forward.restricted then result.is_access_restricted = self.tmp.main.forward.restricted end
 else
   result.forward_mode = 0
   result.forward_speed = 0
 end
 
 if self.tmp.main.backward.mode then
   result.backward_mode = self.tmp.main.backward.mode
   if self.tmp.main.backward.speed then result.backward_speed = self.tmp.main.backward.speed end
   if self.tmp.main.backward.restricted then result.is_access_restricted = self.tmp.main.backward.restricted end
 else
   result.backward_mode = 0
   result.backward_speed = 0
 end
end

-- Pre-process both directions together

function Base:pre_process_combined()
  if self:initial_check() == false then
    return false
  end
  self:preselect_mode()
end

-- Post-process both directions together

function Base:post_process_combined()
  self:handle_narrow_ways()
  self:handle_roundabout()
  self:handle_name()
  self:handle_startpoint()
end

function Base:handle_startpoint()
  if self:is_bidirectional() then
    self.tmp.main.common.startpoint = true
  end
end

-- Process a single direction (forward or backward)

function Base:process_direction(d)
  self:debug( d, "check " .. self.settings.name )

  local ok =
    self:handle_access(d) ~= false and 
    self:handle_blocking(d) ~= false and
    self:handle_oneway(d) ~= false and
    self:handle_speed(d) ~= false and
    self:handle_default_speed(d) ~= false and
    self:handle_speed_adjustment(d) ~= false
  
  self:handle_mode(d)
end

-- If the main mode is not possible use this method to try other modes,
-- like pushing bikes (foot) for a bicycle profile.

function Base:handle_alternative_mode(d)
  -- override handle things like pushing bikes
end

-- Determine the mode

function Base:handle_mode(d)
  if d.denied or d.blocked then
    self:handle_alternative_mode(d)
  elseif d.granted or d.implied or self.tmp.main.common.route then
    if self.tmp.main.common.route then
      d.mode = self.tmp.main.common.route.mode
    else
      d.mode = self.settings.mode
    end
  else
    self:debug( d, self.settings.name .. " impossible" )
    self:handle_alternative_mode(d)
  end
end

-- Early check whether something dictates a different mode (like ferry or train)

function Base:preselect_mode()
  return self:handle_route()
end

-- Intial check to quickly abandon ways that cannot be used for routing

function Base:initial_check()
  if self.settings.ignore_areas and self:tag('area') == 'yes' then
    self:debug( self.tmp.main.forward, "ignoring area")
    self:debug( self.tmp.main.backward, "ignoring area")
    self.tmp.main.common.area = true
    return false
  end
  if self.settings.ignore_buildings and self:tag('building') == 'yes' then
    self:debug( self.tmp.main.forward, "ignoring building")
    self:debug( self.tmp.main.backward, "ignoring building")
    self.tmp.main.common.building = true
    return false
  end
end

-- Check if something is blocking access
-- Note: access tags are handles in another method

function Base:handle_blocking(d)
  if self:is_nonexisting(self:tag('highway')) or
     self:tag('impassable') == 'yes' or
     self:tag('status') == 'impassable' or
     self:tag('smoothness') == 'impassable' then
    self:debug( d, "impassable")
    d.blocked = true
    return false
  end
end

-- Handle routes like ferry and train

function Base:handle_route()
  for key, settings in pairs(self.settings.routes) do
    if self:try_route(key,settings) then
      return true
    else
      local value = self:tag(key)
      if self:is_nonexisting(value) then
        self.tmp.main.forward.denied = true
        self.tmp.main.backward.denied = true
      end
    end      
  end
end

-- Try to handle a specific route, like ferry or train

function Base:try_route(key,settings)
  local value = self:tag(key)
  local r = settings[value]
  if r then
    local access_key, access_value = self:find_access_tag()
    self.tmp.main.common.route = r
    
    self:debug( self.tmp.main.forward, "using " .. value, key)
    self:debug( self.tmp.main.backward, "using " .. value, key)

    self:handle_duration(r.speed)
    self:handle_name()
    return true
  end
end

-- Handle the duration tag that is used for routes

function Base:handle_duration(speed)
  local duration = Duration.parse( self:tag('duration') )
  if duration then
    self.tmp.main.common.duration = duration
  else
    self.tmp.main.forward.speed = speed
    self.tmp.main.backward.speed = speed
  end
end

-- Handle movable bridges, as a separate mode

function Base:handle_movable_bridge()
  if self:tag('bridge') == 'movable' then
    local mode = self.settings.routes.movable_bridge.mode or self.settings.mode
    local speed = self.settings.routes.movable_bridge.speed
    self.tmp.main.forward.mode = mode
    self.tmp.main.backward.mode = mode
    self:handle_duration(speed)
    return true
  end
end

-- Traverse our access tag hierachy and return the first key/value pair found

function Base:find_access_tag()
  for i,key in ipairs(self.settings.access.tags) do
    local v = self:tag(key)
    if v then
      return key, v
    end
  end
  return 'access', self:tag('access')
end

-- Traverse our access tag hierachy and return the first key/value pair found,
-- also looking for directional tags

function Base:find_directional_access_tag(d)
  for v,access_key in ipairs(self.settings.access.tags) do
    local key, tag = self:find_directional_tag( d, access_key )
    if tag then
      return key, tag
    end
  end
  return self:find_directional_tag( d, 'access' )
end

-- For a specific tag key, look for directional tags

function Base:find_directional_tag(d,key)
  local tag = self:tag(key)
  if tag and tag ~= '' then
    return key, tag
  end
  if d then
    local tag = self:tag(key .. ':' .. d.direction)
    if tag and tag ~= '' then
      return key .. ':' .. d.direction, tag
    end
  end
end
-- For a specific base tag, traverse our access tag hierachy and return 
-- the first key/value pair found. Look for both mode:tag and tag:mode
-- e.g. for a car look for oneway:motorcar, oneway:motor_vehicle, oneway:vehicle and then oneway.
-- Setting abort_at will stop the search when this point is reached in in the access hierachy,
-- effectively looking only at tags that are at least as specific as that. 

function Base:determine_tag(base_tag, hierachy, abort_at)
  for i,modifier in ipairs(hierachy) do
    local key = base_tag .. ':' .. modifier
    local tag = self:tag(key)
    if tag and tag ~= '' then
      return key, tag
    end
    key = modifier .. ':' .. base_tag
    tag = self:tag(key)
    if tag and tag ~= '' then
      return key, tag
    end
    if abort_at and modifier == abort_at then
      return
    end
  end
  local tag = self:tag(base_tag)
  if tag and tag ~= '' then
    return base_tag, tag
  end
end

-- Add a string to the debug message list

function Base:debug(d,str,key,value)
  if key ~= nil and value ~= nil then
    table.insert(d.debug, str .. ' (' .. key .. '=' .. value .. ')')
  elseif key ~= nil then
    table.insert(d.debug, str .. ' (' .. key .. '=' .. self:tag(key) .. ')')
  else
    table.insert(d.debug, str)
  end
end

-- Handle access tags

function Base:handle_access(d)
  if d.denied or d.granted then
    return
  end
      
  local key, access = self:find_directional_access_tag(d)
  if access then
    if Base.is_tag_in_table( access, self.settings.access.blacklist ) then
      self:debug( d, "access denied", key, access)
      d.denied = true
      d.mode = nil
      return false
    end
    
    if Base.is_tag_in_table( access, self.settings.access.whitelist ) then
      self:debug( d, "access granted", key, access)
      d.granted = true
    end
    
    if Base.is_tag_in_table( access, self.settings.access.restricted ) or
       Base.is_tag_in_table( self:tag('service'), self.settings.service_tag_restricted ) then
         self:debug( d, "access restricted", key, access)
      d.restricted = true
    end
  elseif self.tmp.main.common.route then 
    if self.tmp.main.common.route.require_access == true then
      self:debug( d, "access missing")
      d.denied = true
      d.mode = nil
      return false
    end   
  end
end

-- Handle name

function Base:handle_name()
  if self:tag('ref') and '' ~= self:tag('ref') and self:tag('name') and '' ~= self:tag('name') then
    self.tmp.main.common.name = self:tag('name') .. ' (' .. self:tag('ref') .. ')'
  elseif self:tag('ref') and '' ~= self:tag('ref') then
    self.tmp.main.common.name = self:tag('ref')
  elseif self:tag('name') and '' ~= self:tag('name') then
    self.tmp.main.common.name = self:tag('name')
  elseif self.settings.use_fallback_names and self:tag('highway') then
    -- If no name or ref present, then encode the highway type, 
    -- so front-ends can at least indicate the type of way to the user.
    -- This encoding scheme is excepted to be a temporary solution.
    self.tmp.main.common.name = '{highway:' .. self:tag('highway') .. '}'
  end
end

-- Handle roundabout flag

function Base:handle_roundabout()
  if self:tag('junction') == 'roundabout' then
    self.tmp.main.common.roundabout = true
  end
end

-- Determine speed based on way type

function Base:handle_speed(d)
  if d.implied or d.granted then
    return
  end
  
  local speed = self.settings.way_speeds[self:tag('highway')]
  if speed then
    self:debug( d, self.settings.name .." access implied", 'highway' )
    d.implied = true
    self:debug( d, "speed is " .. speed .. " based on way type", 'highway' )
    d.speed = speed
  else
    for k,accepted_values in pairs(self.settings.accessible) do
      local v = self:tag(k)
      if accepted_values[v] then
        self:debug( d, "accessible", k,v )
        d.implied = true
      end
    end
  end
end

-- If nothing more specific was been determined, then use a default speed
  
function Base:handle_default_speed(d)
  if self.tmp.main.common.route then
    if self.tmp.main.common.duration == nil then
      self:debug( d, "using route speed of " .. self.tmp.main.common.route.speed)
      d.speed = self.tmp.main.common.route.speed
    else
      self:debug( d, "duration instead of speed")
    end
  elseif not d.speed then
    if d.granted or d.implied then
      self:debug( d, "using default speed of " .. self.settings.default_speed)
      d.speed = self.settings.default_speed
    end
  end
end

function Base:limit_speed_using_table(d,k,table)
  if d.speed and k and table then
    local v = self:tag(k)
    local max = table[v]
    if v and max then
      self:limit_speed( d, max, k,value )
    end
  end
end

function Base:limit_speed(d,max,k,value)
  if d.speed and max and max>0 and d.speed > max then
    self:debug( d, "limiting speed from " .. d.speed .. " to " .. max, k,value)
    d.speed = max
  end
end

function Base:scale_speed(d,v)
  if d.speed and v then
    d.speed = d.speed * v
  end
end

function Base:increase_speed(d,v)
  if d.speed and v then
    d.speed = d.speed + v
  end
end

-- Adjust speed based on things like surface

function Base:handle_speed_adjustment(d)
  self:handle_maxspeed(d)
  self:handle_side_roads_reduction(d)
  self:limit_speed_using_table( d, 'surface', self.settings.surface_speeds )
  self:limit_speed_using_table( d, 'tracktype', self.settings.tracktype_speeds )
  self:limit_speed_using_table( d, 'smoothness', self.settings.smoothness_speeds )
end

function Base:handle_side_roads_reduction(d)
  if self.settings.side_road_speed_reduction then
    if self:tag('side_road') == 'yes' or self:tag('side_road') == 'rotary' then
      self:debug( d, "reducing speed on side road")
      self:scale_speed( d, self.settings.side_road_speed_reduction )
    end
  end
end

-- Is this a bidirectional way
function Base:is_bidirectional()
  return self.tmp.main.forward.mode ~= Base.modes.none and self.tmp.main.backward.mode ~= Base.modes.none
end

-- Adjust speed on narrow roads

function Base:handle_narrow_ways()
  if self.settings.maxspeed_narrow and self:is_bidirectional() then
    local width = Base.extract_int(self:tag('width'))
    local lanes = Base.extract_int(self:tag('lanes'))
    local key, value
    
    if lanes and lanes==1 then
      key, value = 'lanes', lanes
    elseif width and width<=3 then
      key, value = 'width', width
    end  
    
    if key and value then
      self:debug( self.tmp.main.forward, 'narrow street', key, value)
      self:limit_speed( self.tmp.main.forward, self.settings.maxspeed_narrow )
      self:debug( self.tmp.main.backward, 'narrow street', key, value)
      self:limit_speed( self.tmp.main.backward, self.settings.maxspeed_narrow )
    end
  end
end

-- Estimate speed based on maxspeed value

function Base:maxspeed_to_real_speed(maxspeed)
  local factor = self.settings.maxspeed_factor or 1
  local delta = self.settings.maxspeed_delta or 0
  return (maxspeed * factor) + delta
end

function Base:handle_maxspeed_tag(d,key)
  local value = self:tag(key)
  if value == nil then
    return
  end
  
  local parsed = self:parse_maxspeed( value )
  if parsed == nil then
    return
  end

  local adjusted = self:maxspeed_to_real_speed(parsed)
  if adjusted == nil then
    return
  end
  
  if self.settings.maxspeed_reduce and self.settings.maxspeed_increase then
    self:debug( d, "setting speed to " .. adjusted, key, value)
    d.speed = adjusted
    return true
  elseif self.settings.maxspeed_reduce and (d.speed == nil or adjusted < d.speed) then
    self:debug( d, "reducing speed to " .. adjusted, key, value)
    d.speed = adjusted      
    return true    
  elseif self.settings.maxspeed_increase and (d.speed == nil or adjusted > d.speed) then
    self:debug( d, "increasing speed to " .. adjusted, key, value)
    d.speed = adjusted 
    return true         
  end
end

-- Handle various maxspeed tags

function Base:handle_maxspeed(d)
  if not (self.settings.maxspeed_reduce or self.settings.maxspeed_increase) then
    return
  end
  
  keys = {
    'maxspeed:advisory:' .. d.direction,
    'maxspeed:' .. d.direction,
    'maxspeed:advisory',
    'maxspeed',
  }
  for i,key in ipairs(keys) do
    if self:handle_maxspeed_tag( d, key ) then
      return
    end
  end
end

-- Parse a maxspeed tag value.

function Base:parse_maxspeed(tag)
  if Base.empty(tag) then
    return
  end
  local n = tonumber(tag:match("%d*"))
  if n then
    -- parse direct values like 90, 90 km/h, 40mph
    if string.match(tag, 'mph') or string.match(tag, 'mp/h') then
      n = n * Convert.MilesPerHourToKmPerHour
    end
  else
    -- parse defaults values like FR:urban, using the specified table of defaults and overrides
    if self.settings.maxspeed_defaults then
      tag = string.lower(tag)
      n = self.settings.maxspeed_overrides[tag]
      if not n then
        local highway_type = string.match(tag, "%a%a:(%a+)")
        n = self.settings.maxspeed_defaults[highway_type]
      end
    end
  end
  return n
end

-- Handle oneways

function Base:handle_oneway(d)
  if d.oneway == false then
    return
  end
  if self.settings.oneway_handling == nil or self.settings.oneway_handling == false then
    return
  end
  local key, oneway = self:determine_tag( 'oneway', self.settings.access.tags, self.settings.oneway_handling )
  if d.direction == 'forward' then
    if oneway == '-1' then
      self:debug( d, "access denied by reverse oneway", key, oneway)
      d.denied = true
    end
  elseif d.direction == 'backward' then
    if oneway == '-1' then
      self:debug( d, "not considering implied oneway due to reverse oneway", 'oneway')
    elseif oneway == 'yes' or oneway == '1' or oneway == 'true' then
      self:debug( d, "access denied by explicit oneway", key, oneway)
      d.denied = true
    elseif self.settings.oneway_handling == true and self:is_implied_oneway( d, oneway ) then
      self:implied_oneway(d)
    end    
  end
  if d.denied then
    d.mode = nil
    return false
  end
end

-- Implied oneway found

function Base:implied_oneway(d)
  self:debug( d, "access denied by implied oneway" )
  d.denied = true
end

-- Check if oneway is implied by way type

function Base:is_implied_oneway(d,oneway)
  return self.settings.oneway_handling == true and oneway ~= 'no' and (
    self:tag('junction') == 'roundabout' or
    self:tag('highway') == 'motorway_link' or
    self:tag('highway') == 'motorway'
  )
end

-- Try processing the direction using an alternative mode.
-- Mode should be a subclass of Base.

function Base:try_alternative_mode(mode,d)
  if mode.initial_check(mode) ~= false then
    local direction
    if d.direction == 'forward' then
      direction = mode.tmp.main.forward
    else
      direction = mode.tmp.main.backward
    end

    mode.process_direction(mode,direction)
    if direction.mode then
      d.mode = direction.mode
      d.speed = direction.mode
      return true
    end
  end
end

-- Main entry point for node processing

function Base:process_node(node, result)
  self.tags = node
  if self:handle_node_access(result) then
    self:handle_node_barriers(result)
  end
  self:handle_node_traffic_lights(result)
end

-- Handle access for node

function Base:handle_node_access(result)
  local key, access = self:find_directional_access_tag()
  if Base.is_tag_in_table( access, self.settings.access.blacklist ) then
    result.barrier = true
  elseif Base.is_tag_in_table( access, self.settings.access.whitelist ) then
  else
    return true
  end
end

-- Handle traffic light flag for node

function Base:handle_node_barriers(result)
  if result.barrier ~= true then
    local barrier = self:tag('barrier')
    if Base.present(barrier) and not Base.is_tag_in_table( barrier, self.settings.barrier_whitelist ) then
      local bollard = self:tag('bollard')
      if self:tag('bollard') ~= 'rising' then
        result.barrier = true
      end
    end
  end
end

-- Handle traffic light flag

function Base:handle_node_traffic_lights(result)
  if self:tag('highway') == "traffic_signals" then
    result.traffic_lights = true
  end
end

-- Compute turn penalty as angle^2 so that sharp turns incur a much higher cost

function Base:turn(angle)
  if self.settings.turn_penalty then
    return angle * angle * self.settings.turn_penalty/(90.0*90.0)
  else
    return 0
  end
end
  
return Base