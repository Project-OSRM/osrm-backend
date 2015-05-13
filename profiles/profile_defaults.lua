barrier_whitelist = { ["cattle_grid"] = true, ["border_control"] = true, ["checkpoint"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["lift_gate"] = true, ["no"] = true, ["entrance"] = true }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestry"] = true, ["emergency"] = true, ["psv"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierachy = { "motorcar", "motor_vehicle", "vehicle", "access" }
restriction_exception_tags = { "motorcar", "motor_vehicle", "vehicle" }
service_tag_restricted = { ["parking_aisle"] = true }
ignore_in_grid = { ["ferry"] = true }

vehicle_highway_max_speeds = { ["*"] = 130 }
vehicle_hierarchy = { "motorcar", "motor_vehicle", "vehicle" }

vehicle_height = 2
vehicle_width = 2
vehicle_length = 5

vehicle_weight = 3

access_mode_explicit_allowance = 1
access_mode_explicit_forbiddance = 2

access_mode = access_mode_explicit_forbiddance


dimension_check = true
weight_check = true


-- http://wiki.openstreetmap.org/wiki/Speed_limits
-- http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Maxspeed
speed_profile = {
  ["urban"] = 50,
  ["rural"] = 90,
  ["motorway"] = 110,
  ["motorway_link"] = 80,
  ["trunk"] = 85,
  ["trunk_link"] = 40,
  ["primary"] = 65,
  ["primary_link"] = 30,
  ["secondary"] = 60,
  ["secondary_link"] = 25,
  ["tertiary"] = 40,
  ["tertiary_link"] = 20,
  ["unclassified"] = 25,
  ["residential"] = 25,
  ["living_street"] = 10,
  ["service"] = 15,
  ["track"] = 5,
  ["ferry"] = 5,
  ["shuttle_train"] = 10,
  ["default"] = 10,

  ["at:rural"] = 100,
  ["at:trunk"] = 100,

  ["ch:rural"] = 80,
  ["ch:trunk"] = 100,
  ["ch:motorway"] = 120,

  ["de:urban"] = 50,
  ["de:rural"] = 100,
  ["de:motorway"] = math.huge,
  ["de:motorway:urban"] = math.huge,
  ["de:motorway:rural"] = math.huge,
  ["de:trunk"] = 120,
  ["de:trunk:urban"] = 120,
  ["de:trunk:rural"] = 120,
  ["de:primary"] = 70,
  ["de:primary:urban"] = 50,
  ["de:primary:rural"] = 100,
  ["de:secondary"] = 65,
  ["de:secondary:urban"] = 50,
  ["de:secondary:rural"] = 100,
  ["de:tertiary"] = 55,
  ["de:tertiary:urban"] = 50,
  ["de:tertiary:local"] = 100,
  ["de:residential"] = 30,
  ["de:living_street"] = 7,

  ["gb:nsl_single"] = (60 * 1609) / 1000,
  ["gb:nsl_dual"] = (70 * 1609) / 1000,
  ["gb:motorway"] = (70 * 1609) / 1000,
  ["uk:nsl_single"] = (60 * 1609) / 1000,
  ["uk:nsl_dual"] = (70 * 1609) / 1000,
  ["uk:motorway"] = (70 * 1609) / 1000,

  ["ro:trunk"] = 100,

  ["ru:living_street"] = 20,
  ["ru:urban"] = 60,
  ["ru:motorway"] = 110,

  ["ua:urban"] = 60
}

-- surface/trackype/smoothness
-- values were estimated from looking at the photos at the relevant wiki pages

-- max speed for surfaces
surface_speeds = {
  ["asphalt"] = nil,    -- nil mean no limit. removing the line has the same effect
  ["concrete"] = nil,
  ["concrete:plates"] = nil,
  ["concrete:lanes"] = nil,
  ["paved"] = nil,

  ["cement"] = 80,
  ["compacted"] = 80,
  ["fine_gravel"] = 80,

  ["paving_stones"] = 60,
  ["metal"] = 60,
  ["bricks"] = 60,

  ["grass"] = 40,
  ["wood"] = 40,
  ["sett"] = 40,
  ["grass_paver"] = 40,
  ["gravel"] = 40,
  ["unpaved"] = 40,
  ["ground"] = 40,
  ["dirt"] = 40,
  ["pebblestone"] = 40,
  ["tartan"] = 40,

  ["cobblestone"] = 30,
  ["clay"] = 30,

  ["earth"] = 20,
  ["stone"] = 20,
  ["rocky"] = 20,
  ["sand"] = 20,

  ["mud"] = 10
}

-- max speed for tracktypes
tracktype_speeds = {
  ["grade1"] =  60,
  ["grade2"] =  40,
  ["grade3"] =  30,
  ["grade4"] =  25,
  ["grade5"] =  20
}

-- max speed for smoothnesses
smoothness_speeds = {
  ["intermediate"]    =  80,
  ["bad"]             =  40,
  ["very_bad"]        =  20,
  ["horrible"]        =  10,
  ["very_horrible"]   =  5,
  ["impassable"]      =  0
}

-- not in use!
traffic_signal_penalty    = 0.5
-- not in use?
use_turn_restrictions     = true

-- not in use?
take_minimum_of_speeds    = false
obey_oneway               = true
ignore_areas              = true
-- not in use?
u_turn_penalty            = 20

speed_reduction 		  = 0.95
speed_reduction_add		  = 0
