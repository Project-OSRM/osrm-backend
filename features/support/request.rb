require 'net/http'

HOST = 'http://localhost:5000'

def request_path path
  @query = path
  log path
  uri = URI.parse "#{HOST}/#{path}"
  Net::HTTP.get_response uri
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end
