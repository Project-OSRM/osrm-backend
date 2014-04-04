When(/^I run "osrm\-routed\s?(.*?)"$/) do |options|
  Dir.chdir TEST_FOLDER do
    if options.include? '{base}'
      # expand {base} to base path of preprocessed data file
      raise "*** Cannot expand {base} without a preprocessed file." unless @osm_file
      options_expanded = options.gsub "{base}", "#{@osm_file}"
    else
      options_expanded = options
    end

    begin
      Timeout.timeout(1) do
        @stdout = `#{BIN_PATH}/osrm-routed #{options_expanded} 2>error.log`
        @stderr = File.read 'error.log'
        @exit_code = $?.exitstatus
      end
    rescue Timeout::Error
      raise "*** osrm-routed didn't quit. Maybe the --trial option wasn't used?"
    end
  end
end

Then /^it should exit with code (\d+)$/ do |code|
  @exit_code.should == code.to_i
end

Then /^stdout should contain "(.*?)"$/ do |str|
  @stdout.should include(str)
end

Then /^stderr should contain "(.*?)"$/ do |str|
  @stderr.should include(str)
end

Then(/^stdout should contain \/(.*)\/$/) do |regex_str|
  regex = Regexp.new regex_str
  @stdout.should =~ regex
end

Then(/^stderr should contain \/(.*)\/$/) do |regex_str|
  regex = Regexp.new regex_str
  @stderr.should =~ regex
end

Then /^stdout should be empty$/ do
  @stdout.should == ""
end

Then /^stderr should be empty$/ do
  @stderr.should == ""
end

Then /^stdout should contain (\d+) lines?$/ do |lines|
  @stdout.lines.count.should == lines.to_i
end
