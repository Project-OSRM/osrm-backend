Before do |scenario|
  @scenario_title = scenario.title
  @scenario_time = Time.now.strftime("%Y-%m-%dT%H:%m:%SZ")
  reset_data
  @has_logged_preprocess_info = false
  @has_logged_scenario_info = false
end

Around('@routing') do |scenario, block|
  Timeout.timeout(10) do
    block.call
  end
end

After do
  osrm_kill
end
