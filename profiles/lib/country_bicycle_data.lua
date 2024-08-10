-- Assigns extra_speeds list based on location tag
-- based on reading https://wiki.openstreetmap.org/wiki/Key:motorroad
-- and https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions
-- (esp #Alternative_ideas)
-- We treat all cases of motorroad="yes" as no access.
-- pass in way data and speed to set.
-- 

local Set = require('lib/set')
local Sequence = require('lib/sequence')




local country_bicycle_data = {}

local no_speed = -1

local default_speed = 15
local walking_speed = 5

local bicycle_speeds = Sequence {
    motorway       = no_speed,
    cycleway       = default_speed,
    primary        = default_speed,
    primary_link   = default_speed,
    trunk          = default_speed,
    trunk_link     = default_speed,
    secondary      = default_speed,
    secondary_link = default_speed,
    tertiary       = default_speed,
    tertiary_link  = default_speed,
    residential    = default_speed,
    unclassified   = default_speed,
    living_street  = default_speed,
    road           = default_speed,
    service        = default_speed,
    track          = 12,
    path           = 13,
    path           = 13,
    busway         = default_speed,
    bus            = default_speed,
    bridleway      = default_speed,
    footway        = default_speed,
    steps          = walking_speed,
    pier           = walking_speed
  }


local clist = Set
{
  'Worldwide',
  'AUS',
  'AUT',
  'tyrol',
  'BLR',
  'BEL',
  'BRA',
  'CHN',
  'DNK',
  'FRA',
  'FIN',
  'DEU',
  'GRC',
  'HKG',
  'HUN',
  'ISL',
  'IRL',
  'ITA',
  'NLD',
  'NOR',
  'OMN',
  'PHL',
  'POL',
  'ROU',
  'RUS',
  'SVK',
  'ESP',
  'SWE',
  'CHE',
  'THA',
  'TUR',
  'UKR',
  'GBR',
  'USA'
}

local cnames = Sequence
{
  Australia = "AUS",
  Austria = "AUT",
  Belarus = "BLR",
  Belgium = "BEL",
  Brazil = "BRA",
  China = "CHN",
  Denmark = "DNK",
  France = "FRA",
  Finland = "FIN",
  Germany = "DEU",
  Greece = "GRC",
  Hong_Kong = "HKG",
  Hungary = "HUN",
  Iceland = "ISL",
  Ireland = "IRL",
  Italy = "ITA",
  Netherlands = "NLD",
  Norway = "NOR",
  Oman = "OMN",
  Philippines = "PHL",
  Poland = "POL",
  Romania = "ROU",
  Russia = "RUS",
  Slovakia = "SVK",
  Spain = "ESP",
  Sweden = "SWE",
  Switzerland = "CHE",
  Thailand = "THA",
  Turkey = "TUR",
  Ukraine = "UKR",
  United_Kingdom = "GBR",
  United_States_of_America = "USA"
}

local countries = Sequence
{
  Worldwide = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  AUS = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    },

    bicycle_nsw = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    },

    bicycle_vic = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  AUT = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        road = bicycle_speeds.road,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        pier = bicycle_speeds.pier
      }
    }
  },

  tyrol = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        road = bicycle_speeds.road,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        pier = bicycle_speeds.pier
      }
    }
  },

  BLR = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        pedestrian = bicycle_speeds.pedestrian,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        footway_sidewalk = bicycle_speeds.footway_sidewalk,
        footway_crossing = bicycle_speeds.footway_crossing,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        pier = bicycle_speeds.pier
      }
    }
  },

  BEL = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        track = bicycle_speeds.track,
        busway = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  BRA = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = bicycle_speeds.motorway,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        road = bicycle_speeds.road,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pedestrian = bicycle_speeds.pedestrian,
        footway = bicycle_speeds.footway,
        cycleway = bicycle_speeds.cycleway,
        bridleway = bicycle_speeds.bridleway,
        path = bicycle_speeds.path,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        pier = bicycle_speeds.pier
      }
    }
  },

  CHN = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  DNK = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  FRA = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        track = bicycle_speeds.track,
        living_street = bicycle_speeds.living_street,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  FIN = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  DEU = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        track = bicycle_speeds.track,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        pier = bicycle_speeds.pier
      }
    }
  },

  GRC = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  HKG = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  HUN = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        road = bicycle_speeds.road,
        living_street = bicycle_speeds.living_street,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  ISL = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  IRL = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  ITA = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        path = bicycle_speeds.path,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = bicycle_speeds.pedestrian,
        track = bicycle_speeds.track,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        service = bicycle_speeds.service,
        tertiary = bicycle_speeds.tertiary,
        secondary = bicycle_speeds.secondary,
        primary = bicycle_speeds.primary,
        trunk = bicycle_speeds.trunk,
        motorway = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        living_street = bicycle_speeds.living_street,
        pier = bicycle_speeds.pier
      }
    }
  },

  NLD = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        service = bicycle_speeds.service,
        road = bicycle_speeds.road,
        track = bicycle_speeds.track,
        living_street = bicycle_speeds.living_street,
        path = bicycle_speeds.path,
        busway = no_speed,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        pier = bicycle_speeds.pier
      }
    }
  },

  NOR = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        pier = bicycle_speeds.pier
      }
    }
  },

  OMN = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = bicycle_speeds.motorway,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  PHL = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  POL = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  ROU = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  RUS = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        footway_sidewalk = bicycle_speeds.footway_sidewalk,
        footway_crossing = bicycle_speeds.footway_crossing,
        steps = bicycle_speeds.steps,
        road = bicycle_speeds.road,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  SVK = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        road = bicycle_speeds.road,
        living_street = bicycle_speeds.living_street,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  ESP = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        track = bicycle_speeds.track,
        living_street = bicycle_speeds.living_street,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = bicycle_speeds.pedestrian,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  SWE = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        path = bicycle_speeds.path,
        cycleway = bicycle_speeds.cycleway,
        bridleway = bicycle_speeds.bridleway,
        footway = bicycle_speeds.footway,
        pedestrian = bicycle_speeds.pedestrian,
        busway = bicycle_speeds.busway,
        bus = bicycle_speeds.bus,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        pier = bicycle_speeds.pier
      }
    }
  },

  CHE = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        track = bicycle_speeds.track,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        pier = bicycle_speeds.pier
      }
    }
  },

  THA = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = bicycle_speeds.motorway_link,
        trunk = bicycle_speeds.trunk,
        trunk_link = bicycle_speeds.trunk_link,
        primary = bicycle_speeds.primary,
        primary_link = bicycle_speeds.primary_link,
        secondary = bicycle_speeds.secondary,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary = bicycle_speeds.tertiary,
        tertiary_link = bicycle_speeds.tertiary_link,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        steps = no_speed,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  TUR = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = no_speed,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  UKR = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        track = bicycle_speeds.track,
        road = bicycle_speeds.road,
        living_street = bicycle_speeds.living_street,
        service = bicycle_speeds.service,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = no_speed,
        cycleway = bicycle_speeds.cycleway,
        footway = bicycle_speeds.footway,
        steps = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        pier = bicycle_speeds.pier
      }
    }
  },

  GBR = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        pedestrian = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        road = bicycle_speeds.road,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  },

  USA = Sequence
  {
    bicycle = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = bicycle_speeds.trunk,
        primary = bicycle_speeds.primary,
        secondary = bicycle_speeds.secondary,
        tertiary = bicycle_speeds.tertiary,
        unclassified = bicycle_speeds.unclassified,
        residential = bicycle_speeds.residential,
        living_street = bicycle_speeds.living_street,
        road = bicycle_speeds.road,
        pedestrian = bicycle_speeds.pedestrian,
        path = bicycle_speeds.path,
        bridleway = bicycle_speeds.bridleway,
        cycleway = bicycle_speeds.cycleway,
        footway = no_speed,
        primary_link = bicycle_speeds.primary_link,
        secondary_link = bicycle_speeds.secondary_link,
        tertiary_link = bicycle_speeds.tertiary_link,
        service = bicycle_speeds.service,
        track = bicycle_speeds.track,
        pier = bicycle_speeds.pier
      }
    }
  }
}


function country_bicycle_data.inAccessSet(country)
  if clist[country] then
    return true
  end
  return false
end

function country_bicycle_data.getCnameEntry(country)
  if cnames[country] then
    return cnames[country]
  end
  nob = string.gsub(country, ' ', '_')
  if cnames[nob] then
    return cnames[nob]
  end
  return false
end

function country_bicycle_data.getAccessProfile(country, profile)
  if clist[country] then
    if countries[country][profile] then
      return countries[country][profile] 
    end
  end
  return countries['Worldwide'][profile]
end

return country_bicycle_data

