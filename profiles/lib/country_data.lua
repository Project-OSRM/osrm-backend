-- Country lookup utilities for country-specific access profiles
local Set = require('lib/set')
local Sequence = require('lib/sequence')
local country_data = {}

local clist = Set {
  'Worldwide',
  'AUS', 'AUT', 'tyrol', 'BLR', 'BEL', 'BRA', 'CHN', 'DNK',
  'FRA', 'FIN', 'DEU', 'GRC', 'HKG', 'HUN', 'ISL', 'IRL',
  'ITA', 'NLD', 'NOR', 'OMN', 'PHL', 'POL', 'ROU', 'RUS',
  'SVK', 'ESP', 'SWE', 'CHE', 'THA', 'TUR', 'UKR', 'GBR', 'USA'
}

local cnames = {
  Australia = "AUS", Austria = "AUT", Belarus = "BLR", Belgium = "BEL",
  Brazil = "BRA", China = "CHN", Denmark = "DNK", France = "FRA",
  Finland = "FIN", Germany = "DEU", Greece = "GRC", Hong_Kong = "HKG",
  Hungary = "HUN", Iceland = "ISL", Ireland = "IRL", Italy = "ITA",
  Netherlands = "NLD", Norway = "NOR", Oman = "OMN", Philippines = "PHL",
  Poland = "POL", Romania = "ROU", Russia = "RUS", Slovakia = "SVK",
  Spain = "ESP", Sweden = "SWE", Switzerland = "CHE", Thailand = "THA",
  Turkey = "TUR", Ukraine = "UKR", United_Kingdom = "GBR",
  United_States_of_America = "USA"
}

function country_data.inAccessSet(country)
  return clist[country] ~= nil
end

function country_data.getCnameEntry(country)
  if cnames[country] then return cnames[country] end
  local normalized = string.gsub(country, ' ', '_')
  return cnames[normalized]
end

return country_data
