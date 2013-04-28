require 'net/http'

HOST = "http://localhost:#{OSRM_PORT}"
REQUEST_TIMEOUT = 1
DESTINATION_REACHED = 15      #OSRM instruction code

class Hash
  def to_param(namespace = nil)
    collect do |key, value|
      "#{key}=#{value}"
    end.sort
  end
end

def request_path path, waypoints=[], options={}
  locs = waypoints.compact.map { |w| "loc=#{w.lat},#{w.lon}" }
  params = (locs + options.to_param).join('&')
  params = nil if params==""
  uri = URI.parse ["#{HOST}/#{path}", params].compact.join('?')
  Timeout.timeout(REQUEST_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

def request_route waypoints, params={}
  defaults = { 'output' => 'json', 'instructions' => true, 'alt' => true }
  request_path "viaroute", waypoints, defaults.merge(params)
end

def parse_response response
  if response.code == "200" && response.body.empty? == false
    json = JSON.parse response.body
    if json['status'] == 0
      route = way_list json['route_instructions']
      if route.empty?
        "Empty route: #{json['route_instructions']}"
      else
        "Route: #{route}"
      end
    elsif json['status'] == 207
      "No route"
    else
      "Status: #{json['status']}"
    end
  else
    "HTTP: #{response.code}"
  end
end

def got_route? response
  if response.code == "200" && !response.body.empty?
    json = JSON.parse response.body
    if json['status'] == 0
      return way_list( json['route_instructions']).empty? == false
    end
  end
  false
end

def route_status response
  if response.code == "200" && !response.body.empty?
    json = JSON.parse response.body
    if json['status'] == 0
      if way_list( json['route_instructions']).empty?
        return 'Empty route'
      else
        return 'x'
      end
    elsif json['status'] == 207
      ''
    else
      "Status #{json['status']}"
    end
  else
    "HTTP #{response.code}"
  end
end

def way_list instructions
  instructions.reject { |r| r[0].to_s=="#{DESTINATION_REACHED}" }.
  map { |r| r[1] }.
  map { |r| r=="" ? '""' : r }.
  join(',')
end

def compass_list instructions
  instructions.reject { |r| r[0].to_s=="#{DESTINATION_REACHED}" }.
  map { |r| r[6] }.
  map { |r| r=="" ? '""' : r }.
  join(',')
end

def bearing_list instructions
  instructions.reject { |r| r[0].to_s=="#{DESTINATION_REACHED}" }.
  map { |r| r[7] }.
  map { |r| r=="" ? '""' : r }.
  join(',')
end

def turn_list instructions
  types = {
    0 => :none,
    1 => :straight,
    2 => :slight_right,
    3 => :right,
    4 => :sharp_right,
    5 => :u_turn,
    6 => :sharp_left,
    7 => :left,
    8 => :slight_left,
    9 => :via,
    10 => :head,
    11 => :enter_roundabout,
    12 => :leave_roundabout,
    13 => :stay_roundabout,
    14 => :start_end_of_street,
    15 => :destination
  }
  instructions.
  map { |r| types[r[0].to_i].to_s }.
  join(',')
end

def mode_list instructions
  instructions.reject { |r| r[0].to_s=="#{DESTINATION_REACHED}" }.
  map { |r| r[8] }.
  map { |r| (r=="" || r==nil) ? '""' : r }.
  join(',')
end