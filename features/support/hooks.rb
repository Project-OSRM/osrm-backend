
STRESS_TIMEOUT = 300


Before do |scenario|

  # fetch scenario and feature name, so we can use it in log files if needed
  case scenario
    when Cucumber::RunningTestCase::Scenario
      @feature_name = scenario.feature.name
      @scenario_title = scenario.name
    when Cucumber::RunningTestCase::ExampleRow
      @feature_name = scenario.scenario_outline.feature.name
      @scenario_title = scenario.scenario_outline.name
  end
  
  @load_method  = DEFAULT_LOAD_METHOD
  @query_params = {}
  @scenario_time = Time.now.strftime("%Y-%m-%dT%H:%m:%SZ")
  reset_data
  @has_logged_preprocess_info = false
  @has_logged_scenario_info = false
  set_grid_size DEFAULT_GRID_SIZE
  set_origin DEFAULT_ORIGIN

end

Around('@stress') do |scenario, block|
  Timeout.timeout(STRESS_TIMEOUT) do
    block.call
  end
end

After do
end
