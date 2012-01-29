
def request_route a,b
  uri = URI.parse "http://localhost:5000/viaroute&start=#{a}&dest=#{b}&output=json&geomformat=cmp"
  #puts "routing: #{uri}"
  Net::HTTP.get_response uri
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed didn't respond."
end

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
  puts @response.body
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
  computed_route.should == sanitize_route(route)
end

Then /^the route should not follow "([^"]*)"$/ do |route|
  computed_route.should_not == sanitize_route(route)
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

def parse_response response
  if response.code == "200" && response.body.empty? == false
    json = JSON.parse response.body
    puts response.body
    if json['status'] == 0
      route = json['route_instructions'].map { |r| r[1] }.reject(&:empty?).join(', ')
      if route.empty?
        "Empty route: #{json['route_instructions']}"
      else
        route
      end
    elsif json['status'] == 207
      nil #no route found
    else
      "Status: #{json['status']}"
    end
  else
    "HTTP: #{response.code}"
  end
end


When /^I route I should get$/ do |table|
  actual = []
  reprocess_if_needed
  Dir.chdir 'test' do
    launch
    table.hashes.each do |row|
      from_node = @name_node_hash[ row['from'] ]
      to_node = @name_node_hash[ row['to'] ]
      route = parse_response( request_route("#{from_node.lon},#{from_node.lat}", "#{to_node.lat},#{to_node.lat}") )
      actual << { 'from' => row['from'].dup, 'to' => row['to'].dup, 'route' => route }
    end
    kill
  end
  table.diff! actual
 end

When /^I route on tagged ways I should get $/ do |table|
  pending
end


When /^I speak I should get$/ do |table|
  actual = [['one','two','three']]
  table.hashes.each do |row|
    actual << [ row['one'].dup, row['two'].dup, 'xx' ]
  end
  table.diff! actual
end
  
When /^I route I between "([^"]*)" and "([^"]*)"$/ do |from, to|
end

Then /^I should get the route "([^"]*)"$/ do |route|
  route.should == "xx"
end

  