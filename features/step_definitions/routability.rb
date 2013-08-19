Then /^routability should be$/ do |table|
  build_ways_from_table table
  reprocess
  actual = []
  if table.headers&["forw","backw","bothw"] == []
    raise "*** routability tabel must contain either 'forw', 'backw' or 'bothw' column"
  end
  OSRMLauncher.new("#{@osm_file}.osrm") do
    table.hashes.each_with_index do |row,i|
      got = row.dup
      attempts = []
      ['forw','backw','bothw'].each do |direction|
        if table.headers.include? direction
          if direction == 'forw' || direction == 'bothw'
            a = Location.new @origin[0]+(1+WAY_SPACING*i)*@zoom, @origin[1]
            b = Location.new @origin[0]+(3+WAY_SPACING*i)*@zoom, @origin[1]
            response = request_route [a,b]
          elsif direction == 'backw' || direction == 'bothw'
            a = Location.new @origin[0]+(3+WAY_SPACING*i)*@zoom, @origin[1]
            b = Location.new @origin[0]+(1+WAY_SPACING*i)*@zoom, @origin[1]
            response = request_route [a,b]
          end
          want = shortcuts_hash[row[direction]] || row[direction]     #expand shortcuts
          got[direction] = route_status response
          json = JSON.parse(response.body)
          if got[direction].empty? == false
            route = way_list json['route_instructions']
            if route != "w#{i}"
              got[direction] = ''
            elsif want =~ /^\d+s/
              time = json['route_summary']['total_time']
              got[direction] = "#{time}s"
            end
          end
          if FuzzyMatch.match got[direction], want
            got[direction] = row[direction]
          else
            attempts << { :attempt => direction, :query => @query, :response => response }
          end
        end
      end
      log_fail row,got,attempts if got != row
      actual << got
    end
  end
  table.routing_diff! actual
end
