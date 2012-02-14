def log s='', type=nil
  if type == :preprocess
    file = PREPROCESS_LOG_FILE
  else
    file = LOG_FILE
  end
  File.open(file, 'a') {|f| f.write("#{s}\n") }
end


def log_scenario_fail_info
  return if @has_logged_scenario_info
  log "========================================="
  log "Failed scenario: #{@scenario_title}"
  log "Time: #{@scenario_time}"
  log
  log '```xml' #so output can be posted directly to github comment fields
  log osm_str.strip
  log '```'
  log
  log speedprofile_str
  log
  @has_logged_scenario_info = true
end

def log_fail expected,actual,failed
  log_scenario_fail_info
  log "== "
  log "Expected: #{expected}"
  log "Got: #{actual}"
  log
  failed.each do |fail|
    log "Attempt: #{fail[:attempt]}"
    log "Query: #{fail[:query]}"
    log "Response: #{fail[:response].body}"
    log
  end
end


def log_preprocess_info
  return if @has_logged_preprocess_info
  log "=========================================", :preprocess
  log "Preprocessing data for scenario: #{@scenario_title}", :preprocess
  log "Time: #{@scenario_time}", :preprocess
  log '', :preprocess
  log "== OSM data:", :preprocess
  log '```xml', :preprocess #so output can be posted directly to github comment fields
  log osm_str, :preprocess
  log '```', :preprocess
  log '', :preprocess
  log "== Speed profile:", :preprocess
  log speedprofile_str.strip, :preprocess
  log '', :preprocess
  @has_logged_preprocess_info = true
end

def log_preprocess str
  log_preprocess_info
  log str, :preprocess
end

def log_preprocess_done
end


