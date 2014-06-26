When /^I request \/(.*)$/ do |path|
  reprocess
  OSRMBackgroundLauncher.new("#{@osm_file}.osrm") do
    @response = request_path path
  end
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
  @json['status'].class.should == Fixnum
end

Then /^status code should be (\d+)$/ do |code|
  @json = JSON.parse @response.body
  @json['status'].should == code.to_i
end

Then /^status message should be "(.*?)"$/ do |message|
  @json = JSON.parse @response.body
  @json['status_message'].should == message
end

Then /^response should be a well-formed route$/ do
  step "response should be well-formed"
  @json['status_message'].class.should == String
  @json['route_summary'].class.should == Hash
  @json['route_geometry'].class.should == String
  @json['route_instructions'].class.should == Array
  @json['via_points'].class.should == Array
  @json['via_indices'].class.should == Array
end

Then /^"([^"]*)" should return code (\d+)$/ do |binary, code|
  @process_error.is_a?(OSRMError).should == true
  @process_error.process.should == binary
  @process_error.code.to_i.should == code.to_i
end
