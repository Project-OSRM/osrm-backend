When /^I request \/(.*)$/ do |path|
  osrm_kill
  reprocess
  OSRMLauncher.new do
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

