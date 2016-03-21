-- Foot processer

local Base = require('lib/base')

Foot = Base:subClass()
local settings = {
  name = 'foot',
  mode = Base.modes.foot,
  oneway_handling = 'foot',
  maxspeed_reduce = false,
  maxspeed_increase = false,
  default_speed = 5,
  access = {
    tags = Sequence { 'foot' },
  },
  barrier_whitelist = Set { 
    'cycle_barrier', 'cattle_grid', 'border_control', 'checkpoint', 'toll_booth', 
    'sally_port', 'gate', 'lift_gate', 'bollard', 'border_control','toll_booth','block'
  },
  surface_speeds = {
    fine_gravel = 4,
    gravel = 4,
    pebblestone = 4,
    earth = 4,
    stone = 3,
    rocky = 3,
    sand = 2.5,
    mud = 2.5,
  },
  way_speeds = {
    primary = 5,
    primary_link = 5,
    secondary = 5,
    secondary_link = 5,
    tertiary = 5,
    tertiary_link = 5,
    unclassified = 5,
    residential = 5,
    road = 5,
    living_street = 5,
    service = 5,
    track = 5,
    path = 5,
    steps = 5,
    pedestrian = 5,
    footway = 5,
    pier = 5
  },
  accessible = {
    railway = Set { 'platform' },
    amenity = Set { 'parking' },
    leisure = Set { 'track' },
  },    
}
Foot.settings = Table.deep_merge(settings,Foot.settings)

function Foot:prepare_processing(tags,result)
  Base.prepare_processing(self,tags,result)
end

function Foot:is_implied_oneway(d,oneway)
  if self:tag('junction') == 'roundabout' then
    return true
  end
  
  return Base.is_implied_oneway(self,d,oneway)
end

return Foot