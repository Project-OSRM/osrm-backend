Given /^the data file "([^"]*)" is present$/ do |file|
  File.exists?(file).should == true
end

When /^I run the extractor with "([^"]*)"$/ do |cmd|
  @response = `#{cmd}`
  #Dir.chdir @test_folder do
  #  @response = IO.popen([cmd, :err=>[:child, :out]]) { |ls_io| ls_result_with_error = ls_io.read }
  #end
end

When /^I run the preprocessor with "([^"]*)"$/ do |cmd|
  @response = `#{cmd}`
end

Given /^the preprocessed files for "([^"]*)" are present and up to date$/ do |area|
  File.exists?("#{area}.osrm").should == true
  File.exists?("#{area}.osrm.names").should == true
  File.exists?("#{area}.osrm.restrictions").should == true
  File.exists?("#{area}.osrm.hsgr").should == true
  File.exists?("#{area}.osrm.nodes").should == true
  File.exists?("#{area}.osrm.ramIndex").should == true
  File.exists?("#{area}.osrm.fileIndex").should == true
end

Then /^I should see the file "([^"]*)"$/ do |file|
  File.exists?(file).should == true
end
