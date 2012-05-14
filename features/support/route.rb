require 'net/http'

DESTINATION_REACHED = 15      #OSRM instruction code


def request_route a,b
  @query = "http://localhost:5000/viaroute?loc=#{a}&loc=#{b}&output=json&geomformat=cmp"
  #log @query
  uri = URI.parse @query
  Net::HTTP.get_response uri
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
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
