
When /^I preprocess data$/ do
  begin
    osrm_kill
    reprocess
  rescue OSRMError => e
    @process_error = e
  end
end

Then /^preparing should return code (\d+)$/ do |code|
  @process_error.class.should == OSRMError
  @process_error.process.should == 'osrm-prepare'
  @process_error.code.to_i.should == code.to_i
end
