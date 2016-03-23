When /^I request range I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{contracted_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      source = find_node_by_name row['source'] #a
      raise "*** unknown in-node '#{row['source']}" unless source

      node = find_node_by_name row['node'] #b
      raise "*** unknown in-node '#{row['node']}" unless node

      pred = find_node_by_name row['pred'] #a
      raise "*** unknown in-node '#{row['pred']}" unless pred

      # dist = row['distance']
      # raise "*** unknown in-node '#{row['distance']}" unless dist

      #print(source_node)
      response = request_range source, @query_params
      if response.code == "200" && response.body.empty? == false
        json = JSON.parse response.body
        # print(json)
        if json['status'] == 200
          size = json['Nodes found']
          range = json['Range-Analysis']
        end
      end

      #print(size)
      # got = {'source' => row['source'],
      #        'node' => p1,
      #        'pred' => p2}
      # 'distance' => retDis}

      # print(range)
      ok = false
      for coordinate in range;
        p1 = Array.new
        p2 = Array.new


        p1 << coordinate['p1']['lat']
        p1 << coordinate['p1']['lon']
        p2 << coordinate['p2']['lat']
        p2 << coordinate['p2']['lon']

        got = {'source' => row['source'],
               'node' => p1,
               'pred' => p2}

        print(p1)
        print(p2)
        print("fuu");

        if FuzzyMatch.match_location(p1, node) && FuzzyMatch.match_location(p2, pred) || ok
          print("asdasdasd")
          key = 'node'
          got[key] = row[key]
          key = 'pred'
          got[key] = row[key]
          ok = true
        else
          key = 'node'
          row[key] = "#{row[key]} [#{node.lat},#{node.lon}]"
          key = 'pred'
          row[key] = "#{row[key]} [#{node.lat},#{node.lon}]"
        end
      end

      # row.keys.each do |key|
      #   if key=='node'
      #     if FuzzyMatch.match_location p1, node
      #       got[key] = row[key]
      #       ok = true
      #     else
      #       row[key] = "#{row[key]} [#{node.lat},#{node.lon}]"
      #     end
      #   end
      #   if key=='pred'
      #     if FuzzyMatch.match_location p2, pred
      #       got[key] = row[key]
      #       ok = true
      #     else
      #       row[key] = "#{row[key]} [#{pred.lat},#{pred.lon}]"
      #     end
      #   end
      # if key=='distance'
      #   if retDis == dist
      #     got[key] = row[key]
      #   else
      #     row[key] = dist
      #     ok = false
      #   end
      # end
      # end

      # end


      # print(p1)
      # print(p2)
      # print(node)



      unless ok
        failed = { :attempt => 'range', :query => @query, :response => response }
        log_fail row,got,[failed]
      end
      actual << got
    end
  end
  table.diff! actual
end
