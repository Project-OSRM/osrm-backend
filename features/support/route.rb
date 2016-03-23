require 'net/http'

HOST = "http://127.0.0.1:#{OSRM_PORT}"
DESTINATION_REACHED = 15      #OSRM instruction code

def request_path service, params
  uri = "#{HOST}/" + service
  response = send_request uri, params
  return response
end

def request_url path
  uri = URI.parse"#{HOST}/#{path}"
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

# Overwriters the default values in defaults.
# e.g. [[a, 1], [b, 2]], [[a, 5], [d, 10]] => [[a, 5], [b, 2], [d, 10]]
def overwrite_params defaults, other
  merged = []
  defaults.each do |k,v|
    idx = other.index { |p| p[0] == k }
    if idx == nil then
      merged << [k, v]
    else
      merged << [k, other[idx][1]]
    end
  end
  other.each do |k,v|
    if merged.index { |pair| pair[0] == k} == nil then
      merged << [k, v]
    end
  end

  return merged
end

def request_route waypoints, bearings, user_params
  raise "*** number of bearings does not equal the number of waypoints" unless bearings.size == 0 || bearings.size == waypoints.size

  defaults = [['output','json'], ['instructions',true], ['alt',false]]
  params = overwrite_params defaults, user_params
  encoded_waypoint = waypoints.map{ |w| ["loc","#{w.lat},#{w.lon}"] }
  if bearings.size > 0
    encoded_bearings = bearings.map { |b| ["b", b.to_s]}
    parasm = params.concat encoded_waypoint.zip(encoded_bearings).flatten! 1
  else
    params = params.concat encoded_waypoint
  end

  return request_path "viaroute", params
end

def request_nearest node, user_params
  defaults = [['output', 'json']]
  params = overwrite_params defaults, user_params
  params << ["loc", "#{node.lat},#{node.lon}"]

  return request_path "nearest", params
  end

def request_range node, user_params
  defaults = [['output', 'json']]
  params = overwrite_params defaults, user_params
  params << ["loc", "#{node.lat},#{node.lon}"]

  return request_path "range", params
end

def request_table waypoints, user_params
  defaults = [['output', 'json']]
  params = overwrite_params defaults, user_params
  params = params.concat waypoints.map{ |w| [w[:type],"#{w[:coord].lat},#{w[:coord].lon}"] }

  return request_path "table", params
end

def request_trip waypoints, user_params
  defaults = [['output', 'json']]
  params = overwrite_params defaults, user_params
  params = params.concat waypoints.map{ |w| ["loc","#{w.lat},#{w.lon}"] }

  return request_path "trip", params
end

def request_matching waypoints, timestamps, user_params
  defaults = [['output', 'json']]
  params = overwrite_params defaults, user_params
  encoded_waypoint = waypoints.map{ |w| ["loc","#{w.lat},#{w.lon}"] }
  if timestamps.size > 0
    encoded_timestamps = timestamps.map { |t| ["t", t.to_s]}
    parasm = params.concat encoded_waypoint.zip(encoded_timestamps).flatten! 1
  else
    params = params.concat encoded_waypoint
  end

  return request_path "match", params
end

def got_route? response
  if response.code == "200" && !response.body.empty?
    json = JSON.parse response.body
    if json['status'] == 200
      return way_list( json['route_instructions']).empty? == false
    end
  end
  return false
end

def route_status response
  if response.code == "200" && !response.body.empty?
    json = JSON.parse response.body
    return json['status']
  else
    "HTTP #{response.code}"
  end
end

def extract_instruction_list instructions, index, postfix=nil
  if instructions
    instructions.reject { |r| r[0].to_s=="#{DESTINATION_REACHED}" }.
    map { |r| r[index] }.
    map { |r| (r=="" || r==nil) ? '""' : "#{r}#{postfix}" }.
    join(',')
  end
end

def way_list instructions
  extract_instruction_list instructions, 1
end

def compass_list instructions
  extract_instruction_list instructions, 6
end

def bearing_list instructions
  extract_instruction_list instructions, 7
end

def turn_list instructions
  if instructions
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
      15 => :destination,
      16 => :name_changes,
      17 => :enter_contraflow,
      18 => :leave_contraflow
    }
    # replace instructions codes with strings
    # "11-3" (enter roundabout and leave a 3rd exit) gets converted to "enter_roundabout-3"
    instructions.map do |r|
      r[0].to_s.gsub(/^\d*/) do |match|
        types[match.to_i].to_s
      end
    end.join(',')
  end
end

def mode_list instructions
  extract_instruction_list instructions, 8
end

def time_list instructions
  extract_instruction_list instructions, 4, "s"
end

def distance_list instructions
  extract_instruction_list instructions, 2, "m"
end
