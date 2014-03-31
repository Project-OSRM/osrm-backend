require 'rspec/expectations'

DEFAULT_PORT = 5000
DEFAULT_TIMEOUT = 2

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

if ENV["OSRM_TIMEOUT"]
  OSRM_TIMEOUT = ENV["OSRM_TIMEOUT"].to_i
  puts "Timeout set to #{OSRM_TIMEOUT}"
else
  OSRM_TIMEOUT = DEFAULT_TIMEOUT
  puts "Using default timeout #{OSRM_TIMEOUT}"
end


AfterConfiguration do |config|
  clear_log_files
end