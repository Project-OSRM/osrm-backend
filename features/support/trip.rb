require 'net/http'
HOST = "http://127.0.0.1:#{OSRM_PORT}"

def request_trip waypoints=[], params={}
  defaults = { 'output' => 'json' }
  locs = waypoints.compact.map { |w| "loc=#{w.lat},#{w.lon}" }

  params = (locs + defaults.merge(params).to_param).join('&')
  params = nil if params==""

  uri = generate_request_url ("trip" + '?' + params)
  response = send_request uri, waypoints, params
end

