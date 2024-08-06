-- Assigns extra_speeds list based on location tag
-- based on reading https://wiki.openstreetmap.org/wiki/Key:motorroad
-- and https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions
-- (esp #Alternative_ideas)
-- We treat all cases of motorroad="yes" as no access.
-- pass in way data and speed to set.
-- 

local Set = require('lib/set')
local Sequence = require('lib/sequence')



local country_foot_data = {}

local walking_speed = 5
local default_speed = 15
local no_speed = -1

local speeds = Sequence {
    motorway        = 90,
    motorway_link   = 45,
    trunk           = 85,
    trunk_link      = 40,
    primary         = 65,
    primary_link    = 30,
    secondary       = 55,
    secondary_link  = 25,
    tertiary        = 40,
    tertiary_link   = 20,
    unclassified    = 25,
    residential     = 25,
    living_street   = 10,
    service         = 15
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
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  AUS = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  AUT = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        road = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  tyrol = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        road = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  BLR = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        pedestrian = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        footway_sidewalk = walking_speed,
        footway_crossing = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  BEL = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        track = walking_speed,
        busway = no_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  BRA = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = walking_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        road = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        track = walking_speed,
        pedestrian = walking_speed,
        footway = walking_speed,
        cycleway = no_speed,
        bridleway = walking_speed,
        path = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  CHN = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  DNK = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  FRA = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        track = walking_speed,
        living_street = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  FIN = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  DEU = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        service = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        track = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  GRC = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  HKG = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        cycleway = no_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  HUN = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        road = walking_speed,
        living_street = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  ISL = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  IRL = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = no_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  ITA = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        path = walking_speed,
        cycleway = no_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        track = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        service = walking_speed,
        tertiary = walking_speed,
        secondary = walking_speed,
        primary = walking_speed,
        trunk = walking_speed,
        motorway = no_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        living_street = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  NLD = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        service = walking_speed,
        road = walking_speed,
        track = walking_speed,
        living_street = walking_speed,
        path = walking_speed,
        busway = no_speed,
        bridleway = no_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  NOR = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        service = walking_speed,
        track = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  OMN = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  PHL = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  POL = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        service = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  ROU = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = no_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = no_speed,
        footway = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  RUS = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        footway_sidewalk = walking_speed,
        footway_crossing = walking_speed,
        steps = walking_speed,
        road = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        track = walking_speed,
        pier = walking_speed
      }
    }
  },

  SVK = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        road = walking_speed,
        living_street = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  ESP = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        track = walking_speed,
        living_street = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  SWE = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        track = walking_speed,
        path = walking_speed,
        cycleway = walking_speed,
        bridleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        busway = no_speed,
        bus = no_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  CHE = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = no_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        track = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  THA = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        motorway_link = walking_speed,
        trunk = walking_speed,
        trunk_link = walking_speed,
        primary = walking_speed,
        primary_link = walking_speed,
        secondary = walking_speed,
        secondary_link = walking_speed,
        tertiary = walking_speed,
        tertiary_link = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        steps = walking_speed,
        service = walking_speed,
        track = walking_speed,
        pier = walking_speed
      }
    }
  },

  TUR = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  UKR = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        track = walking_speed,
        road = walking_speed,
        living_street = walking_speed,
        service = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = walking_speed,
        steps = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        pier = walking_speed
      }
    }
  },

  GBR = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        pedestrian = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        road = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  },

  USA = Sequence
  {
    foot = Sequence
    {
      highway = 
      {
        motorway = no_speed,
        trunk = walking_speed,
        primary = walking_speed,
        secondary = walking_speed,
        tertiary = walking_speed,
        unclassified = walking_speed,
        residential = walking_speed,
        living_street = walking_speed,
        road = walking_speed,
        pedestrian = walking_speed,
        path = walking_speed,
        bridleway = walking_speed,
        cycleway = walking_speed,
        footway = walking_speed,
        primary_link = walking_speed,
        secondary_link = walking_speed,
        tertiary_link = walking_speed,
        service = walking_speed,
        track = walking_speed,
        steps = walking_speed,
        pier = walking_speed
      }
    }
  }
}


function country_foot_data.inAccessSet(country)
  if clist[country] then
    return true
  end
  return false
end

function country_foot_data.getCnameEntry(country)
  if cnames[country] then
    return cnames[country]
  end
  nob = string.gsub(country, ' ', '_')
  if cnames[nob] then
    return cnames[nob]
  end
  return false
end

function country_foot_data.getAccessProfile(country, profile)
  if clist[country] then
    if countries[country][profile] then
      return countries[country][profile] 
    end
  end
  return countries['Worldwide'][profile]
end

return country_foot_data

