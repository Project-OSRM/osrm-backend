Then /^I should get a valid timestamp/ do
  step "I should get a response"
  step "response should be valid JSON"
  step "response should be well-formed"
  @json['timestamp'].class.should == String
  @json['timestamp'].should == OSM_TIMESTAMP
end
