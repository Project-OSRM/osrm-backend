Then /^routability should be$/ do |table|
  build_ways_from_table table
  reprocess
  actual = []
  if table.headers&["forw","backw","bothw"] == []
    raise "*** routability tabel must contain either 'forw', 'backw' or 'bothw' column"
  end
  OSRMLauncher.new do
    table.hashes.each_with_index do |row,i|
      got = row.dup
      attempts = []
      ['forw','backw','bothw'].each do |direction|
        if table.headers.include? direction
          if direction == 'forw' || direction == 'bothw'
            response = request_route("#{ORIGIN[1]},#{ORIGIN[0]+(1+WAY_SPACING*i)*@zoom}","#{ORIGIN[1]},#{ORIGIN[0]+(3+WAY_SPACING*i)*@zoom}")
          elsif direction == 'backw' || direction == 'bothw'
            response = request_route("#{ORIGIN[1]},#{ORIGIN[0]+(3+WAY_SPACING*i)*@zoom}","#{ORIGIN[1]},#{ORIGIN[0]+(1+WAY_SPACING*i)*@zoom}")
          end
          got[direction] = route_status response
          json = JSON.parse(response.body)
          if got[direction].empty? == false
            route = way_list json['route_instructions']
            if route != "w#{i}"
              got[direction] = "testing w#{i}, but got #{route}!?"
            elsif row[direction] =~ /\d+s/
              time = json['route_summary']['total_time']
              got[direction] = "#{time}s"
            end
          end
          if got[direction] != row[direction]
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
