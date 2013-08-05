require 'rspec/expectations'

DEFAULT_PORT = 5000


puts "Ruby version #{RUBY_VERSION}"
unless RUBY_VERSION.to_f >= 1.9
  raise "*** Please upgrade to Ruby 1.9.x to run the OSRM cucumber tests"
end

if ENV["OSRM_PORT"]
  OSRM_PORT = ENV["OSRM_PORT"].to_i
  puts "Port set to #{OSRM_PORT}"
else
  OSRM_PORT = DEFAULT_PORT
  puts "Using default port #{OSRM_PORT}"
end

