require 'pathname'
require 'json'
require 'open4'
require "net/http"
require "uri"

$stdout.sync = true
$server_pipe = nil
$server_running = false

def read_terminal
  return $server_pipe.read_nonblock 10000 rescue nil
end

def launch cmd
  $server_pipe = IO.popen(cmd)
  sleep 2 # so the daemon has a chance to boot
  
  at_exit do
    Process.kill("KILL", $server_pipe.pid)   # clean up the daemon when the tests finish
  end
end

Given /^I am in the test folder$/ do
  @root = Pathname.new(File.dirname(__FILE__)).parent.parent.expand_path
  @test_folder  = "#{@root}/test"
  Dir.chdir @test_folder
end

Given /^the server is configured for bike routing$/ do
  pending # express the regexp above with the code you wish you had
end

Given /^the "([^"]*)" speedprofile is used$/ do |profile|
  FileUtils.cp "speedprofiles/#{profile}.ini", "speedprofile.ini"
end

Then /^the response should include "([^"]*)"$/ do |string|
  @response.include?(string).should_not == nil
end

Then /^the response should include '([^']*)'$/ do |string|
  @response.include?(string).should_not == nil
end

Given /^the server is running$/ do
  unless $server_running
  	step 'a process called "osrm-routed" should be running'
  	@server_running = true
  end
end

When /^I start the server with "([^']*)"$/ do |cmd|
  launch cmd
end

When /^I stop the server$/ do
  Process.kill("KILL", $server_pipe.pid)
  $server_pipe = nil
end

Then /^a process called "([^']*)" should be running$/ do |daemon|
  `ps -eo command | grep #{@test_folder}/#{daemon}`.size.should > 0
end

Then /^a process called "([^']*)" should not be running$/ do |daemon|
  puts `ps -eo command | grep #{@test_folder}/#{daemon}$`
  `ps -eo command | grep #{@test_folder}/#{daemon}$`.size.should == 0
end

Then /^I should see "([^']*)" on the terminal$/ do |string|
  out = read_terminal
  out.should =~ /#{string}/
end

Then /^no error should be reported in terminal$/ do
  read_terminal.should_not =~ /error/
end
