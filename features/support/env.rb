require 'rspec/expectations'

DEFAULT_PORT = 5000

if ENV["OSRM_PORT"]
  OSRM_PORT = ENV["OSRM_PORT"].to_i
  puts "Port set to #{OSRM_PORT}"
else
  OSRM_PORT = DEFAULT_PORT
  puts "Using default port #{OSRM_PORT}"
end