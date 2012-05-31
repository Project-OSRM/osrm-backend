When /^I request a route from ([^"]+) to ([^"]+)$/ do |a,b|
  @response = request_route a,b
  #puts @response.body
  #@response
end

When /^I request a route from "([^"]*)" to "([^"]*)"$/ do |a,b|
  locations = OSMTestParserCallbacks.locations
  raise "Locations hash is empty. To reference nodes by name, please preprocess the test file earlier in the test." unless locations
  raise "Unknown node: #{a}" unless locations[a]
  raise "Unknown node: #{b}" unless locations[b]
  @response = request_route "#{locations[a][0]},#{locations[a][1]}", "#{locations[b][0]},#{locations[b][1]}"
end

Then /^I should get a response/ do
  @response.code.should == "200"
  @response.body.should_not == nil
  @response.body.should_not == ''
end

Then /^response should be valid JSON$/ do
  @json = JSON.parse @response.body
end

Then /^response should be well-formed$/ do
  @json['version'].class.should == Float
  @json['status'].class.should == Fixnum
  @json['status_message'].class.should == String
  @json['route_summary'].class.should == Hash
  @json['route_geometry'].class.should == String
  @json['route_instructions'].class.should == Array
  @json['via_points'].class.should == Array
  @json['transactionId'].class.should == String
end

Then /^a route should be found$/ do
  @json['status'].should == 0
  @json['status_message'].should == "Found route between points"
end

Then /^no route should be found$/ do
  @json['status'].should == 207
  @json['status_message'].should == "Cannot find route between points"
end

Then /^I should get a valid response$/ do
  step "I should get a response"
  step "response should be valid JSON"
  step "response should be well-formed"
 	#step "no error should be reported in terminal"
end

Then /^I should get a route$/ do
  step "I should get a valid response"
  step "a route should be found"
  #puts @response.body
end

Then /^I should not get a route$/ do
  step "I should get a valid response"
  step "no route should be found"
end

Then /^the route should start at "([^']*)"$/ do |name|
  @json['route_summary']['start_point'].should == name
end

Then /^the route should end at "([^']*)"$/ do |name|
  @json['route_summary']['end_point'].should == name
end

Then /^distance should be between (\d+) and (\d+)$/ do |min,max|
  @json['route_summary']['total_distance'].to_i.should >= min.to_i
  @json['route_summary']['total_distance'].to_i.should <= max.to_i
end

Then /^the distance should be close to (\d+)m$/ do |d|
  @json['route_summary']['total_distance'].to_i.should >= d.to_i*0.95
  @json['route_summary']['total_distance'].to_i.should <= d.to_i/0.95
end

Then /^number of instructions should be (\d+)$/ do |n|
  @json['route_instructions'].size.should == n
end

Then /^there should be 1 turn$/ do
  step 'there should be 1 turns'
end

Then /^there should be (\d+) turns$/ do |n|
  @json['route_instructions'].map {|t| t.first}.select {|t| t =~ /^Turn/ }.size.should == n.to_i
end

Then /^there should be more than (\d+) turn$/ do |n|
  @json['route_instructions'].map {|t| t.first}.select {|t| t =~ /^Turn/ }.size.should > n.to_i
end

Then /^there should not be any turns$/ do
  (@json['route_instructions'].size-1).should == 0
end

def sanitize_route route
  route.split(',').map{|w| w.strip}.reject(&:empty?).join(', ')
end
 
def computed_route
  @json['route_instructions'].map { |r| r[1] }.reject(&:empty?).join(', ')
end
   
Then /^the route should follow "([^"]*)"$/ do |route|
  sanitize_route(route).should == computed_route
end

Then /^the route should not follow "([^"]*)"$/ do |route|
  sanitize_route(route).should_not == computed_route
end

Then /^the route should include "([^"]*)"$/ do |route|
  sanitize_route(route).should =~ /#{computed_route}/
end

Then /^the route should not include "([^"]*)"$/ do |route|
  sanitize_route(route).should_not =~ /#{computed_route}/
end

Then /^the route should stay on "([^"]*)"$/ do |way|
  step "the route should start at \"#{way}\""
  step "the route should end at \"#{way}\""
  step "the route should follow \"#{way}\""
  step "there should not be any turns"
end

When /^I route between "([^"]*)" and "([^"]*)"$/ do |from,to|
  reprocess
  Dir.chdir 'test' do
    from_node = name_node_hash[from]
    to_node = name_node_hash[to]
    a = "#{from_node.lon},#{from_node.lat}"
    b = "#{to_node.lon},#{to_node.lat}"
    @route = parse_response( request_route(a,b) )
  end
end

Then /^"([^"]*)" should be returned$/ do |route|
  @route.should == route.split(',').join(',')
end

Then /^routability should be$/ do |table|
  osrm_kill
  build_ways_from_table table
  reprocess
  actual = []
  if table.headers&["forw","backw"] == []
    raise "*** routability tabel must contain either 'forw' or 'backw' column"
  end
  OSRMLauncher.new do
    table.hashes.each_with_index do |row,i|
      got = row.dup
      attempts = []
      ['forw','backw'].each do |direction|
        if table.headers.include? direction
          if direction == 'forw'
            response = request_route("#{ORIGIN[1]},#{ORIGIN[0]+(1+WAY_SPACING*i)*@zoom}","#{ORIGIN[1]},#{ORIGIN[0]+(2+WAY_SPACING*i)*@zoom}")
          elsif direction == 'backw'
            response = request_route("#{ORIGIN[1]},#{ORIGIN[0]+(2+WAY_SPACING*i)*@zoom}","#{ORIGIN[1]},#{ORIGIN[0]+(1+WAY_SPACING*i)*@zoom}")
          end
          got[direction] = route_status response
          json = JSON.parse(response.body)
          if got[direction].empty? == false
            route = way_list json['route_instructions']
            if route != "w#{i}"
              got[direction] = "testing w#{i}, but got #{route}!?"
            elsif row[direction] =~ /\d+s/
              time = json['route_summary']['total_time']
              got[direction] = "#{time}s"
            end
          end
          if got[direction] != row[direction]
            attempts << { :attempt => direction, :query => @query, :response => response }
          end
        end
      end
      log_fail row,got,attempts if got != row
      actual << got
    end
  end
  table.routing_diff! actual
end

When /^I route I should get$/ do |table|
  osrm_kill
  reprocess
  actual = []
  OSRMLauncher.new do
    table.hashes.each_with_index do |row,ri|
      from_node = @name_node_hash[ row['from'] ]
      raise "*** unknown from-node '#{row['from']}" unless from_node
      to_node = @name_node_hash[ row['to'] ]
      raise "*** unknown to-node '#{row['to']}" unless to_node
      response = request_route("#{from_node.lat},#{from_node.lon}", "#{to_node.lat},#{to_node.lon}")
      if response.code == "200" && response.body.empty? == false
        json = JSON.parse response.body
        if json['status'] == 0
          instructions = way_list json['route_instructions']
          bearings = bearing_list json['route_instructions']
          compasses = compass_list json['route_instructions']
        end
      end
      
      got = {'from' => row['from'], 'to' => row['to'] }
      if table.headers.include? 'start'
        got['start'] = instructions ? json['route_summary']['start_point'] : nil
      end
      if table.headers.include? 'end'
        got['end'] = instructions ? json['route_summary']['end_point'] : nil
      end
      if table.headers.include? 'route'
        got['route'] = (instructions || '').strip
        if table.headers.include? 'distance'
          got['distance'] = instructions ? json['route_summary']['total_distance'].to_s : nil
        end
        if table.headers.include? 'time'
          raise "*** time must be specied in seconds. (ex: 60s)" unless row['time'] =~ /\d+s/
          got['time'] = instructions ? "#{json['route_summary']['total_time'].to_s}s" : nil
        end
        if table.headers.include? 'bearing'
          got['bearing'] = bearings
        end
        if table.headers.include? 'compass'
          got['compass'] = compasses
        end
      end
      
      if row != got
        failed = { :attempt => 'route', :query => @query, :response => response }
        log_fail row,got,[failed]
      end
      
      actual << got
    end
  end
  table.routing_diff! actual
end
