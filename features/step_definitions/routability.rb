def test_routability_row i
  result = {}
  ['forw','backw'].each do |direction|
    a = Location.new @origin[0]+(1+WAY_SPACING*i)*@zoom, @origin[1]
    b = Location.new @origin[0]+(3+WAY_SPACING*i)*@zoom, @origin[1]
    r = {}
    r[:response] = request_route direction=='forw' ? [a,b] : [b,a]
    r[:json] = JSON.parse(r[:response].body)

    r[:status] = route_status r[:response]
    if r[:status].empty? == false
      r[:route] = way_list r[:json]['route_instructions']
      
      if r[:route]=="w#{i}"
        r[:time] = r[:json]['route_summary']['total_time']
        r[:distance] = r[:json]['route_summary']['total_distance']
        r[:speed] = r[:time]>0 ? (3.6*r[:distance]/r[:time]).to_i : nil
      else
        # if we hit the wrong way segment, we assume it's
        # because the one we tested was not unroutable
        r = {}
      end
    end
    result[direction] = r
  end
  
  # check if forw and backw returned the same values
  result['bothw'] = {}
  [:status,:time,:distance,:speed].each do |key|
    if result['forw'][key] == result['backw'][key]
      result['bothw'][key] = result['forw'][key]
    else
      result['bothw'][key] = 'diff'
    end
  end
  
  result
end

Then /^routability should be$/ do |table|
  build_ways_from_table table
  reprocess
  actual = []
  if table.headers&["forw","backw","bothw"] == []
    raise "*** routability tabel must contain either 'forw', 'backw' or 'bothw' column"
  end
  OSRMBackgroundLauncher.new("#{@osm_file}.osrm") do
    table.hashes.each_with_index do |row,i|
      got = row.dup
      attempts = []
      result = test_routability_row i
      (['forw','backw','bothw'] & table.headers).each do |direction|
        want = shortcuts_hash[row[direction]] || row[direction]     #expand shortcuts
                
        case want
        when '', 'x'
          got[direction] = result[direction][:status].to_s
        when /^\d+s/
          got[direction] = "#{result[direction][:time]}s"
        when /^\d+ km\/h/
          got[direction] = "#{result[direction][:speed]} km/h"
        end
        
        if FuzzyMatch.match got[direction], want
          got[direction] = row[direction]
        else
#          attempts << { :attempt => direction, :query => @query, :response => result[direction][:response] }
        end
      end
      
      log_fail row,got,attempts if got != row
      actual << got
    end
  end
  table.routing_diff! actual
end
