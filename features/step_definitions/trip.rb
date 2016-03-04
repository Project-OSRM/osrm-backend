When /^I plan a trip I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{contracted_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      if row['request']
        got = {'request' => row['request'] }
        response = request_url row['request']
      else
        params = @query_params
        waypoints = []
        if row['from'] and row['to']
          node = find_node_by_name(row['from'])
          raise "*** unknown from-node '#{row['from']}" unless node
          waypoints << node

          node = find_node_by_name(row['to'])
          raise "*** unknown to-node '#{row['to']}" unless node
          waypoints << node

          got = {'from' => row['from'], 'to' => row['to'] }
          response = request_trip waypoints, params
        elsif row['waypoints']
          row['waypoints'].split(',').each do |n|
            node = find_node_by_name(n.strip)
            raise "*** unknown waypoint node '#{n.strip}" unless node
            waypoints << node
          end
          got = {'waypoints' => row['waypoints'] }
          response = request_trip waypoints, params
        else
          raise "*** no waypoints"
        end
      end

      row.each_pair do |k,v|
        if k =~ /param:(.*)/
          if v=='(nil)'
            params[$1]=nil
          elsif v!=nil
            params[$1]=[v]
          end
          got[k]=v
        end
      end

      if response.body.empty? == false
        json = JSON.parse response.body
      end

      if table.headers.include? 'status'
        got['status'] = json['status'].to_s
      end
      if table.headers.include? 'message'
        got['message'] = json['status_message']
      end
      if table.headers.include? '#'   # comment column
        got['#'] = row['#']           # copy value so it always match
      end

      if response.code == "200"
        if table.headers.include? 'trips'
          sub_trips = json['trips'].compact.map { |sub| sub['via_points']}
        end
      end

      ######################
      ok = true
      encoded_result = ""
      extended_target = ""
      row['trips'].split(',').each_with_index do |sub, sub_idx|
        if sub_idx >= sub_trips.length
          ok = false
          break
        end

        ok = false;
        #TODO: Check all rotations of the round trip
        sub.length.times do |node_idx|
          node = find_node_by_name(sub[node_idx])
          out_node = sub_trips[sub_idx][node_idx]
          if FuzzyMatch.match_location out_node, node
            encoded_result += sub[node_idx]
            extended_target += sub[node_idx]
            ok = true
          else
            encoded_result += "? [#{out_node[0]},#{out_node[1]}]"
            extended_target += "#{sub[node_idx]} [#{node.lat},#{node.lon}]"
          end
        end
      end

      if ok
        got['trips'] = row['trips']
        got['via_points'] = row['via_points']
      else
        got['trips'] = encoded_result
        row['trips'] = extended_target
        log_fail row,got, { 'trip' => {:query => @query, :response => response} }
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
        log_fail row,got, { 'trip' => {:query => @query, :response => response} }
      end

      actual << got
    end
  end
  table.diff! actual
end

