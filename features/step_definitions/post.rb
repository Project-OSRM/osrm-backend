When /^I request post I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      request_string = row['request'].split("?")
      got = {'request' => row['request'] }
      response = request_post_url request_string[0], request_string[1]

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

      if table.headers.include? 'status_code'
        # the only thing we want to test is
        # an accepted request
        got['status_code'] = response.code.to_s
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
        log_fail row,got, { 'route' => {:query => @query, :response => response} }
      end

      actual << got
    end
  end
  table.diff! actual
end