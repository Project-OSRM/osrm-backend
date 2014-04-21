When(/^I run "osrm\-routed\s?(.*?)"$/) do |options|
  begin
    Timeout.timeout(1) { run_bin 'osrm-routed', options }
  rescue Timeout::Error
    raise "*** osrm-routed didn't quit. Maybe the --trial option wasn't used?"
  end
end

When(/^I run "osrm\-extract\s?(.*?)"$/) do |options|
  run_bin 'osrm-extract', options
end

When(/^I run "osrm\-prepare\s?(.*?)"$/) do |options|
  run_bin 'osrm-prepare', options
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
