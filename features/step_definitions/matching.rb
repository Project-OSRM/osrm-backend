When /^I match I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      if row['request']
        got = {'request' => row['request'] }
        response = request_url row['request']
      else
        params = @query_params
        trace = []
        timestamps = []
        if row['trace']
          row['trace'].each_char do |n|
            node = find_node_by_name(n.strip)
            raise "*** unknown waypoint node '#{n.strip}" unless node
            trace << node
          end
          if row['timestamps']
              timestamps = row['timestamps'].split(" ").compact.map { |t| t.to_i}
          end
          got = {'trace' => row['trace'] }
          response = request_matching trace, timestamps, params
        else
          raise "*** no trace"
        end
      end

      row.each_pair do |k,v|
        if k =~ /param:(.*)/
          if v=='(nil)'
            params[$1]=nil
          elsif v!=nil
            params[$1]=v
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

      sub_matchings = []
      if response.code == "200"
        if table.headers.include? 'matchings'
          sub_matchings = json['matchings'].compact.map { |sub| sub['matched_points']}
        end
      end

      ok = true
      encoded_result = ""
      extended_target = ""
      row['matchings'].split(',').each_with_index do |sub, sub_idx|
        if sub_idx >= sub_matchings.length
          ok = false
          break
        end
        sub.length.times do |node_idx|
          node = find_node_by_name(sub[node_idx])
          out_node = sub_matchings[sub_idx][node_idx]
          if FuzzyMatch.match_location out_node, node
            encoded_result += sub[node_idx]
            extended_target += sub[node_idx]
          else
            encoded_result += "? [#{out_node[0]},#{out_node[1]}]"
            extended_target += "#{sub[node_idx]} [#{node.lat},#{node.lon}]"
            ok = false
          end
        end
      end
      if ok
        got['matchings'] = row['matchings']
        got['timestamps'] = row['timestamps']
      else
        got['matchings'] = encoded_result
        row['matchings'] = extended_target
        log_fail row,got, { 'matching' => {:query => @query, :response => response} }
      end

      actual << got
    end
  end
  table.diff! actual
end

When /^I match with turns I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      if row['request']
        got = {'request' => row['request'] }
        response = request_url row['request']
      else
        params = @query_params
        trace = []
        timestamps = []
        if row['from'] and row['to']
          node = find_node_by_name(row['from'])
          raise "*** unknown from-node '#{row['from']}" unless node
          trace << node

          node = find_node_by_name(row['to'])
          raise "*** unknown to-node '#{row['to']}" unless node
          trace << node

          got = {'from' => row['from'], 'to' => row['to'] }
          response = request_matching trace, timestamps, params
        elsif row['trace']
          row['trace'].each_char do |n|
            node = find_node_by_name(n.strip)
            raise "*** unknown waypoint node '#{n.strip}" unless node
            trace << node
          end
          if row['timestamps']
              timestamps = row['timestamps'].split(" ").compact.map { |t| t.to_i}
          end
          got = {'trace' => row['trace'] }
          response = request_matching trace, timestamps, params
        else
          raise "*** no trace"
        end
      end

      row.each_pair do |k,v|
        if k =~ /param:(.*)/
          if v=='(nil)'
            params[$1]=nil
          elsif v!=nil
            params[$1]=v
          end
          got[k]=v
        end
      end

      if response.body.empty? == false
        json = JSON.parse response.body
      end
      if response.body.empty? == false
        if response.code == "200"
          instructions = way_list json['matchings'][0]['instructions']
          bearings = bearing_list json['matchings'][0]['instructions']
          compasses = compass_list json['matchings'][0]['instructions']
          turns = turn_list json['matchings'][0]['instructions']
          modes = mode_list json['matchings'][0]['instructions']
          times = time_list json['matchings'][0]['instructions']
          distances = distance_list json['matchings'][0]['instructions']
        end
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

      sub_matchings = []
      if response.code == "200"
        if table.headers.include? 'matchings'
          sub_matchings = json['matchings'].compact.map { |sub| sub['matched_points']}

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
          if table.headers.include?('speed')
            if row['speed'] != '' && instructions
              raise "*** Speed must be specied in km/h. (ex: 50 km/h)" unless row['speed'] =~ /\d+ km\/h/
                time = json['route_summary']['total_time']
                distance = json['route_summary']['total_distance']
                speed = time>0 ? (3.6*distance/time).to_i : nil
                got['speed'] =  "#{speed} km/h"
            else
              got['speed'] = ''
            end
          end
          if table.headers.include? 'bearing'
            got['bearing'] = instructions ? bearings : ''
          end
          if table.headers.include? 'compass'
            got['compass'] = instructions ? compasses : ''
          end
          if table.headers.include? 'turns'
            got['turns'] = instructions ? turns : ''
          end
          if table.headers.include? 'modes'
            got['modes'] = instructions ? modes : ''
          end
          if table.headers.include? 'times'
            got['times'] = instructions ? times : ''
          end
          if table.headers.include? 'distances'
            got['distances'] = instructions ? distances : ''
          end
        end
        if table.headers.include? 'start'
          got['start'] = instructions ? json['route_summary']['start_point'] : nil
        end
        if table.headers.include? 'end'
          got['end'] = instructions ? json['route_summary']['end_point'] : nil
        end
        if table.headers.include? 'geometry'
            got['geometry'] = json['route_geometry']
        end
      end

      ok = true
      encoded_result = ""
      extended_target = ""
      row['matchings'].split(',').each_with_index do |sub, sub_idx|
        if sub_idx >= sub_matchings.length
          ok = false
          break
        end
        sub.length.times do |node_idx|
          node = find_node_by_name(sub[node_idx])
          out_node = sub_matchings[sub_idx][node_idx]
          if FuzzyMatch.match_location out_node, node
            encoded_result += sub[node_idx]
            extended_target += sub[node_idx]
          else
            encoded_result += "? [#{out_node[0]},#{out_node[1]}]"
            extended_target += "#{sub[node_idx]} [#{node.lat},#{node.lon}]"
            ok = false
          end
        end
      end
      if ok
        got['matchings'] = row['matchings']
        got['timestamps'] = row['timestamps']
      else
        got['matchings'] = encoded_result
        row['matchings'] = extended_target
        log_fail row,got, { 'matching' => {:query => @query, :response => response} }
      end

      actual << got
    end
  end
  table.diff! actual
end
