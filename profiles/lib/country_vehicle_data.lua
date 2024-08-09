-- Assigns extra_speeds list based on location tag
-- based on reading https://wiki.openstreetmap.org/wiki/Key:motorroad
-- and https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions
-- (esp #Alternative_ideas)
-- We treat all cases of motorroad="yes" as no access.
-- pass in way data and speed to set.
-- 

local Set = require('lib/set')
local Sequence = require('lib/sequence')



country_vehicle_data = {}

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
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  AUS = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  AUT = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        road = speeds.road,
        living_street = speeds.living_street,
        service = speeds.service,
        track = speeds.track,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  tyrol = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        road = speeds.road,
        living_street = speeds.living_street,
        service = speeds.service,
        track = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  BLR = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        service = speeds.service,
        pedestrian = speeds.pedestrian,
        track = speeds.track,
        path = no_speed,
        bridleway = speeds.bridleway,
        cycleway = speeds.cycleway,
        footway = speeds.footway,
        footway_sidewalk = speeds.footway_sidewalk,
        footway_crossing = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  BEL = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        track = speeds.track,
        busway = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  BRA = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        road = speeds.road,
        residential = speeds.residential,
        living_street = speeds.living_street,
        service = speeds.service,
        track = speeds.track,
        pedestrian = no_speed,
        footway = no_speed,
        cycleway = no_speed,
        bridleway = no_speed,
        path = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  CHN = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  DNK = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        track = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  FRA = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        track = speeds.track,
        living_street = speeds.living_street,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = speeds.pedestrian,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  FIN = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        track = speeds.track,
        path = speeds.path,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = speeds.pedestrian,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  DEU = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        service = speeds.service,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        track = speeds.track,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  GRC = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  HKG = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  HUN = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        road = speeds.road,
        living_street = speeds.living_street,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  ISL = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        track = speeds.track,
        path = no_speed,
        bridleway = speeds.bridleway,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  IRL = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  ITA = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        path = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        track = speeds.track,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        service = speeds.service,
        tertiary = speeds.tertiary,
        secondary = speeds.secondary,
        primary = speeds.primary,
        trunk = speeds.trunk,
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        living_street = speeds.living_street
      }
    }
  },

  NLD = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        service = speeds.service,
        road = speeds.road,
        track = speeds.track,
        living_street = speeds.living_street,
        path = no_speed,
        busway = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed
      }
    }
  },

  NOR = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        service = speeds.service,
        track = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  OMN = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  PHL = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  POL = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        service = speeds.service,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  ROU = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = speeds.pedestrian,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        service = speeds.service
      }
    }
  },

  RUS = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        service = speeds.service,
        pedestrian = speeds.pedestrian,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = speeds.footway,
        footway_sidewalk = speeds.footway_sidewalk,
        footway_crossing = no_speed,
        steps = speeds.steps,
        road = speeds.road,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  SVK = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        road = speeds.road,
        living_street = speeds.living_street,
        pedestrian = speeds.pedestrian,
        path = speeds.path,
        bridleway = speeds.bridleway,
        cycleway = speeds.cycleway,
        footway = speeds.footway,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  ESP = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        track = speeds.track,
        living_street = speeds.living_street,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  SWE = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        service = speeds.service,
        track = speeds.track,
        path = speeds.path,
        cycleway = no_speed,
        bridleway = no_speed,
        footway = no_speed,
        pedestrian = speeds.pedestrian,
        busway = no_speed,
        bus = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  CHE = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        track = speeds.track,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  THA = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        motorway_link = speeds.motorway_link,
        trunk = speeds.trunk,
        trunk_link = speeds.trunk_link,
        primary = speeds.primary,
        primary_link = speeds.primary_link,
        secondary = speeds.secondary,
        secondary_link = speeds.secondary_link,
        tertiary = speeds.tertiary,
        tertiary_link = speeds.tertiary_link,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        steps = no_speed,
        service = speeds.service
      }
    }
  },

  TUR = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  UKR = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        track = speeds.track,
        road = speeds.road,
        living_street = speeds.living_street,
        service = speeds.service,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        steps = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link
      }
    }
  },

  GBR = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        pedestrian = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  },

  USA = Sequence
  {
    vehicle = Sequence
    {
      highway = 
      {
        motorway = speeds.motorway,
        trunk = speeds.trunk,
        primary = speeds.primary,
        secondary = speeds.secondary,
        tertiary = speeds.tertiary,
        unclassified = speeds.unclassified,
        residential = speeds.residential,
        living_street = speeds.living_street,
        road = speeds.road,
        pedestrian = no_speed,
        path = no_speed,
        bridleway = no_speed,
        cycleway = no_speed,
        footway = no_speed,
        motorway_link = speeds.motorway_link,
        trunk_link = speeds.trunk_link,
        primary_link = speeds.primary_link,
        secondary_link = speeds.secondary_link,
        tertiary_link = speeds.tertiary_link,
        service = speeds.service
      }
    }
  }
}


function country_vehicle_data.inAccessSet(country)
  if clist[country] then
    return true
  end
  return false
end

function country_vehicle_data.getCnameEntry(country)
  if cnames[country] then
    return cnames[country]
  end
  nob = string.gsub(country, ' ', '_')
  if cnames[nob] then
    return cnames[nob]
  end
  return false
end

function country_vehicle_data.getAccessProfile(country, profile)
  if clist[country] then
    if countries[country][profile] then
      return countries[country][profile] 
    end
  end
  return countries['Worldwide'][profile]
end

return country_vehicle_data

