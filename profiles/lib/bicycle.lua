-- Bicycle processer

local Base = require('lib/base')
local Foot = require('lib/foot')

Bicycle = Base:subClass()
local settings = {
  name = 'bicycle',
  mode = Base.modes.bike,
  default_speed = 15,
  turn_penalty = 10,
  turn_bias = 1.2,         -- invert for countries with left-side driving
  oneway_handling = true,
  maxspeed_reduce = true,
  maxspeed_increase = false,
  access = {
    tags = Sequence { 'bicycle', 'vehicle' },
  },
  barrier_whitelist = Set { 
    'cycle_barrier', 'cattle_grid', 'border_control', 'checkpoint', 'toll_booth', 
    'sally_port', 'gate', 'lift_gate', 'bollard', 'border_control','toll_booth','block'
  },
	way_speeds = {
    track = 12,
    path = 12,
  }, 
  surface_speeds = {
    asphalt = 15,
    ['cobblestone:flattened'] = 10,
    paving_stones = 10,
    compacted = 10,
    cobblestone = 6,
    unpaved = 6,
    fine_gravel = 6,
    gravel = 6,
    pebblestone = 6,
    ground = 6,
    dirt = 6,
    earth = 6,
    grass = 6,
    mud = 3,
    sand = 3,
  },
  
   -- no need to include access by foot here, since it's
   -- handled by the foot profile
  accessible = {  
    highway = Set {
      'cycleway', 
      'primary', 
      'primary_link', 
      'secondary', 
      'secondary_link', 
      'tertiary', 
      'tertiary_link', 
      'residential', 
      'unclassified', 
      'living_street', 
      'road', 
      'service'
    },
    amenity = Set { 'parking' },
    leisure = Set { 'track' },
    man_made = Set { 'pier' },
  },
  cycleway_whitelist = Set { 'lane', 'track', 'shared', 'share_busway', 'sharrow' },
  cycleway_opposite = Set { 'opposite', 'opposite_lane', 'opposite_track' },
}
Bicycle.settings = Table.deep_merge(settings,Bicycle.settings)

function Bicycle:prepare_processing(tags,result)
  Base.prepare_processing(self,tags,result)
  
  -- also prepare foot profile, since we might need it later
  Foot.prepare_processing(Foot,tags,result)
  self.tmp.foot = Foot.tmp.main
  Foot.tmp.main.forward.debug = self.tmp.main.forward.debug
  Foot.tmp.main.backward.debug = self.tmp.main.backward.debug
end

function Bicycle:handle_default_speed(d)
  local surface = self:tag('surface')
  if surface then
    local speed = self.settings.surface_speeds[surface]
    if speed then
      d.speed = speed
      self:debug( d, "speed determined by surface", 'surface' )
      return
    end
  end
  return Base.handle_default_speed(self,d)
end

function Bicycle:handle_access(d) 
  if self:tag('bicycle') == 'dismount' then
    d.denied = true
    return false
  else
    return Base.handle_access(self,d)
  end
end

function Bicycle:handle_oneway(d)
  if self:handle_cycleway(d) == false
    then return false
  end
  return Base.handle_oneway(self,d)
end

function Bicycle:implied_oneway(d)
  Base.implied_oneway(self,d)
  
  if self:tag('junction') == 'roundabout' then
    d.alternative = false
    self:debug( d, "don't walk in roundabout", key, tag )
  end
end

function Bicycle:handle_cycleway(d)
  -- FIXME this assumes right-side driving
  local key,tag,hierachy,opposite,side
  local cycleway = self:tag('cycleway')
  local cycleway_left = self:tag('cycleway:left')
  local cycleway_right = self:tag('cycleway:right')
  local oneway = self:tag('oneway')
  
  if d.direction == 'forward' then
    hierachy = Sequence { 'right' }
    side = cycleway_right
  else
    hierachy = Sequence { 'left' }
    side = cycleway_left
  end
  
  key,tag = Base.determine_tag( self, 'cycleway', hierachy )
  
  if Base.is_tag_in_table(side, self.settings.cycleway_whitelist ) or
     Base.is_tag_in_table(side, self.settings.cycleway_opposite ) then
    d.implied = true
    if d.direction == 'backward' then
      d.oneway = false
    end
    self:debug( d, "cycleway present", key, tag )
  elseif Base.is_tag_in_table(tag, self.settings.cycleway_whitelist ) then
      d.implied = true
      self:debug( d, "cycleway present", key, tag )
  elseif Base.is_tag_in_table(cycleway, self.settings.cycleway_opposite ) then
    if d.direction == 'backward' or oneway == '-1' then
      d.implied = true
      d.oneway = false
      self:debug( d, "opposite cycleway", key, tag )
    end
  end
end

function Bicycle:handle_alternative_mode(d)
  if d.alternative ~= false then
    return self:try_alternative_mode(Foot,d)
  end
end

return Bicycle