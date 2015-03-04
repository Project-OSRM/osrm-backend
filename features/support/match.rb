require 'net/http'

HOST = "http://127.0.0.1:#{OSRM_PORT}"
DESTINATION_REACHED = 15      #OSRM instruction code

class Hash
  def to_param(namespace = nil)
    collect do |key, value|
      "#{key}=#{value}"
    end.sort
  end
end

def request_matching trace=[], timestamps=[], options={}
  defaults = { 'output' => 'json' }
  locs = waypoints.compact.map { |w| "loc=#{w.lat},#{w.lon}" }
  ts = timestamps.compact.map { |t| "t=#{t}" }
  params = (locs + ts + defaults.merge(options).to_param).join('&')
  params = nil if params==""
  uri = URI.parse ["#{HOST}/match", params].compact.join('?')
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

