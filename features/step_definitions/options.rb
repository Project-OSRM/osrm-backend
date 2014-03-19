When(/^I run "osrm\-routed\s?(.*?)"$/) do |options|
  Dir.chdir TEST_FOLDER do
    begin
      Timeout.timeout(1) do
        @stdout = `#{BIN_PATH}/osrm-routed #{options} 2>error.log`
        @stderr = File.read 'error.log'
        @exit_code = $?.exitstatus
      end
    rescue
      # TODO would be easy to handle there was an option to make osrm exit right after completing initialization
      @stdout = nil
      @stderr = nil
      @exit_code = nil
    end
  end
end

Then /^it should exit with code (\d+)$/ do |code|
  @exit_code.should == code.to_i
end

Then /^stdout should contain "(.*?)"$/ do |str|
  @stdout.include?(str).should == true
end

Then /^stderr should contain "(.*?)"$/ do |str|
  @stderr.include?(str).should == true
end

Then /^stdout should be empty$/ do
  @stdout.should == ""
end

Then /^stderr should be empty$/ do
  @stderr.should == ""
end
