def parse_output response, format
  unless response.body.empty?
    if format=='pbf'
      parsed = JSON.parse Protobuffer_response::Route_response.decode(response.body).to_json
      
      
      # fix up data format to match json output
      # hopefully formats can converge so we avoid this step
      parsed['main_route'].each_pair {|k,v| parsed[k]=v }
      parsed.delete 'main_route'
      instructions = []
      parsed['route_instructions'].each  do |v|
        instructions[v['position']] = [
          v['instruction_id'],     # 0
          v['street_name'],        # 1
          v['length'],             # 2
          v['position'],           # 3
          v['time'],               # 4
          v['length_str'],         # 5
          v['earth_direction'],    # 6
          v['azimuth'],            # 7
          v['travel_mode']         # 8
        ]
      end
      parsed['route_instructions'] = instructions
      parsed
    else
      parsed = JSON.parse response.body
    end
  end
end