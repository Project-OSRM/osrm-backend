-- Assigns extra_speeds list based on location tag
-- based on reading https://wiki.openstreetmap.org/wiki/Key:motorroad
-- and https://wiki.openstreetmap.org/wiki/OSM_tags_for_routing/Access_restrictions
-- (esp #Alternative_ideas)
-- We treat all cases of motorroad="yes" as no access.
-- pass in way data and speed to set.
-- 

local Set = require('lib/set')
local Sequence = require('lib/sequence')

local country_data = {}

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

function country_data.inAccessSet(country)
  if clist[country] then
    return true
  end
  return false
end

function country_data.getCnameEntry(country)
  if cnames[country] then
    return cnames[country]
  end
  nob = string.gsub(country, ' ', '_')
  if cnames[nob] then
    return cnames[nob]
  end
  return false
end

function country_data.getAccessProfile(country, profile)
  if clist[country] then
    if countries[country][profile] then
      return countries[country][profile] 
    end
  end
  return countries['Worldwide'][profile]
end

return country_data

