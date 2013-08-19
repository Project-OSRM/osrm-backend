When /^I request \/(.*)$/ do |path|
  reprocess
  OSRMLauncher.new("#{@osm_file}.osrm") do
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
  @json['version'].class.should == Float
  @json['status'].class.should == Fixnum
  @json['transactionId'].class.should == String
end

Then /^response should be a well-formed route$/ do
  step "response should be well-formed"
  @json['status_message'].class.should == String
  @json['route_summary'].class.should == Hash
  @json['route_geometry'].class.should == String
  @json['route_instructions'].class.should == Array
  @json['via_points'].class.should == Array
end

When /^I preprocess data$/ do
  begin
    reprocess
  rescue OSRMError => e
    @process_error = e
  end
end

Then /^"([^"]*)" should return code (\d+)$/ do |binary, code|
  @process_error.is_a?(OSRMError).should == true
  @process_error.process.should == binary
  @process_error.code.to_i.should == code.to_i
end
