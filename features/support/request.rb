require 'net/http'

HOST = "http://localhost:#{OSRM_PORT}"
REQUEST_TIMEOUT = 1

def request_path path
  @query = path
  log path
  uri = URI.parse "#{HOST}/#{path}"
  Timeout.timeout(REQUEST_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end
