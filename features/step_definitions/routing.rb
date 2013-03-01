When /^I route I should get$/ do |table|
  reprocess
  actual = []
  OSRMLauncher.new do
    table.hashes.each_with_index do |row,ri|
      from_node = @name_node_hash[ row['from'] ]
      raise "*** unknown from-node '#{row['from']}" unless from_node
      to_node = @name_node_hash[ row['to'] ]
      raise "*** unknown to-node '#{row['to']}" unless to_node
      response = request_route("#{from_node.lat},#{from_node.lon}", "#{to_node.lat},#{to_node.lon}")
      if response.code == "200" && response.body.empty? == false
        json = JSON.parse response.body
        if json['status'] == 0
          instructions = way_list json['route_instructions']
          bearings = bearing_list json['route_instructions']
          compasses = compass_list json['route_instructions']
          turns = turn_list json['route_instructions']
        end
      end
      
      got = {'from' => row['from'], 'to' => row['to'] }
      if table.headers.include? 'start'
        got['start'] = instructions ? json['route_summary']['start_point'] : nil
      end
      if table.headers.include? 'end'
        got['end'] = instructions ? json['route_summary']['end_point'] : nil
      end
      if table.headers.include? 'route'
        got['route'] = (instructions || '').strip
        if table.headers.include?('distance')
          if row['distance']!=''
            raise "*** Distance must be specied in meters. (ex: 250m)" unless row['distance'] =~ /\d+m/
          end
          got['distance'] = instructions ? "#{json['route_summary']['total_distance'].to_s}m" : ''
        end
        if table.headers.include?('time')
          raise "*** Time must be specied in seconds. (ex: 60s)" unless row['time'] =~ /\d+s/
          got['time'] = instructions ? "#{json['route_summary']['total_time'].to_s}s" : ''
        end
        if table.headers.include? 'bearing'
          got['bearing'] = bearings
        end
        if table.headers.include? 'compass'
          got['compass'] = compasses
        end
        if table.headers.include? 'turns'
          got['turns'] = turns
        end
        if table.headers.include? '#'   # comment column
          got['#'] = row['#']           # copy value so it always match
        end
      end
      
      ok = true
      row.keys.each do |key|
        if FuzzyMatch.match got[key], row[key]
          got[key] = row[key]
        else
          ok = false
        end
      end
      
      unless ok
        failed = { :attempt => 'route', :query => @query, :response => response }
        log_fail row,got,[failed]
      end
      
      actual << got
    end
  end
  table.routing_diff! actual
end

When /^I route (\d+) times I should get$/ do |n,table|
  ok = true
  n.to_i.times do
    ok = false unless step "I route I should get", table
  end
  ok
end
