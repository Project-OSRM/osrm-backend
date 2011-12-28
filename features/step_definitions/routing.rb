
def request_route a,b
  uri = URI.parse "http://localhost:5000/viaroute&start=#{a}&dest=#{b}&output=json&geomformat=cmp"
  Net::HTTP.get_response uri
rescue Errno::ECONNREFUSED => e
  raise "NOTE: The OSRM server is not running. Start it manully, or include tests that start it."
end

When /^I request a route from (.+) to (.+)$/ do |a,b|
  @response = request_route a,b
end

When /^I request a route from "([^"]*)" to "([^"]*)"$/ do |a,b|
  pending
  #store hash og adress => coordinate points in a file under version control
  #if the adress is not found, then use nominatim to get one and store it in the file
  
  #TODO convert a/b from adress to coordinate  
  #@response = request_route a,b
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
 	step "no error should be reported in terminal"
end

Then /^I should get a route$/ do
  step "I should get a valid response"
  step "a route should be found"
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

