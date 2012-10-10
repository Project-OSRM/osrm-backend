-- Begin of globals
barrier_whitelist = { [""] = true, ["bollard"] = true, ["entrance"] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["no"] = true, ["sally_port"] = true, ["gate"] = true}
access_tag_whitelist = { ["yes"] = true, ["permissive"] = true, ["designated"] = true	}
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierachy = { "bicycle", "vehicle", "access" }
cycleway_tags = {["track"]=true,["lane"]=true,["opposite"]=true,["opposite_lane"]=true,["opposite_track"]=true,["share_busway"]=true,["sharrow"]=true,["shared"]=true }
service_tag_restricted = { ["parking_aisle"] = true }
ignore_in_grid = { ["ferry"] = true }

speed_profile = { 
	["cycleway"] = 18,
	["primary"] = 17,
	["primary_link"] = 17,
	["secondary"] = 18,
	["secondary_link"] = 18,
	["tertiary"] = 18,
	["tertiary_link"] = 18,
	["residential"] = 18,
	["unclassified"] = 16,
	["living_street"] = 16,
	["road"] = 16,
	["service"] = 16,
	["track"] = 13,
	["path"] = 13,
	["footway"] = 5,
	["pedestrian"] = 5,
	["pier"] = 5,
	["steps"] = 1,
	["default"] = 18,
	["ferry"] = 5,
	["train"] = 80,
	["railway"] = 60,
	["subway"] = 50,
	["light_rail"] = 40,
	["monorail"] = 40,
	["tram"] = 40
}

take_minimum_of_speeds 	= true
obey_oneway 			= true
obey_bollards 			= false
use_restrictions 		= true
ignore_areas 			= true -- future feature
traffic_signal_penalty 	= 2
u_turn_penalty 			= 20

-- End of globals

--find first tag in access hierachy which is set
function find_access_tag(source)
	for i,v in ipairs(access_tags_hierachy) do 
		local tag = source.tags:Find(v)
		if tag ~= '' then --and tag ~= "" then
			return tag
		end
	end
	return nil
end

function node_function (node)
	local barrier = node.tags:Find ("barrier")
	local access = find_access_tag(node)
	local traffic_signal = node.tags:Find("highway")
	
	-- flag node if it carries a traffic light	
	if traffic_signal == "traffic_signals" then
		node.traffic_light = true
	end
	
	-- parse access and barrier tags
	if access  and access ~= "" then
		if access_tag_blacklist[access] then
			node.bollard = true
		else
			node.bollard = false
		end
	elseif barrier and barrier ~= "" then
		if barrier_whitelist[barrier] then
			node.bollard = false
		else
			node.bollard = true
		end
	end
	
	return 1
end

function way_function (way, numberOfNodesInWay)
	-- A way must have two nodes or more
	if(numberOfNodesInWay < 2) then
		return 0;
	end
	
	-- First, get the properties of each way that we come across
	local highway = way.tags:Find("highway")
	local name = way.tags:Find("name")
	local ref = way.tags:Find("ref")
	local junction = way.tags:Find("junction")
	local route = way.tags:Find("route")
	local railway = way.tags:Find("railway")
	local maxspeed = parseMaxspeed(way.tags:Find ( "maxspeed") )
	local man_made = way.tags:Find("man_made")
	local barrier = way.tags:Find("barrier")
	local oneway = way.tags:Find("oneway")
	local onewayClass = way.tags:Find("oneway:bicycle")
	local cycleway = way.tags:Find("cycleway")
	local cycleway_left = way.tags:Find("cycleway:left")
	local cycleway_right = way.tags:Find("cycleway:right")
	local duration	= way.tags:Find("duration")
	local service	= way.tags:Find("service")
	local area = way.tags:Find("area")
	local access = find_access_tag(way)
	
	-- only route on things with highway tag set (not buildings, boundaries, etc)
    if (not highway or highway == '') and (not route or route == '') and (not railway or railway=='') then
		return 0
    end
		
 	-- access
    if access_tag_blacklist[access] then
		return 0
    end

	-- name	
	if "" ~= ref then
		way.name = ref
	elseif "" ~= name then
		way.name = name
	else
		way.name = highway		-- if no name exists, use way type
	end
	
    if (speed_profile[route] and speed_profile[route] > 0) or (speed_profile[man_made] and speed_profile[man_made] > 0) then
		-- ferries and piers
		if durationIsValid(duration) then
		way.speed = math.max( duration / math.max(1, numberOfNodesInWay-1) )
		 	way.is_duration_set = true;
		end
		way.direction = Way.bidirectional;
		if speed_profile[route] ~= nil then
		  	highway = route;
		elseif speed_profile[man_made] ~= nil then
		  	highway = man_made;
		end
		if not way.is_duration_set then
		 	way.speed = speed_profile[highway]
		end
    elseif railway and speed_profile[railway] then
	 	-- trains and subways
		if access and access_tag_whitelist[access] then
			way.speed = speed_profile[railway]		
			way.direction = Way.bidirectional
		end
	else
		-- ways
		if speed_profile[highway] then 
	      	way.speed = speed_profile[highway]
	    elseif access_tag_whitelist[access] then
			way.speed = speed_profile["default"]
		end
	end
	
	-- maxspeed
	if take_minimum_of_speeds then
		if maxspeed and maxspeed>0 then
			way.speed = math.min(way.speed, maxspeed)
		end
	end
	
	-- direction
	way.direction = Way.bidirectional
	local impliedOneway = false
	if junction == "roundabout" or highway == "motorway_link" or highway == "motorway" then
		way.direction = Way.oneway
		impliedOneway = true
	end
	
	if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
		way.direction = Way.oneway
	elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" then
		way.direction = Way.bidirectional
	elseif onewayClass == "-1" then
		way.direction = Way.opposite
	elseif oneway == "no" or oneway == "0" or oneway == "false" then
		way.direction = Way.bidirectional
	elseif cycleway and string.find(cycleway, "opposite") == 1 then
		if impliedOneway then
			way.direction = Way.opposite
		else
			way.direction = Way.bidirectional
		end
	elseif cycleway_left and cycleway_tags[cycleway_left] and cycleway_right and cycleway_tags[cycleway_right] then
		way.direction = Way.bidirectional
	elseif cycleway_left and cycleway_tags[cycleway_left] then
		if impliedOneway then
			way.direction = Way.opposite
		else
			way.direction = Way.bidirectional
		end
	elseif cycleway_right and cycleway_tags[cycleway_right] then
		if impliedOneway then
			way.direction = Way.oneway
		else
			way.direction = Way.bidirectional
		end
	elseif oneway == "-1" then
		way.direction = Way.opposite
	elseif oneway == "yes" or oneway == "1" or oneway == "true" then
		way.direction = Way.oneway
	end
	
	-- cycleways
	if cycleway and cycleway_tags[cycleway] then
		way.speed = speed_profile["cycleway"]
	elseif cycleway_left and cycleway_tags[cycleway_left] then
		way.speed = speed_profile["cycleway"]
	elseif cycleway_right and cycleway_tags[cycleway_right] then
		way.speed = speed_profile["cycleway"]
	end

	way.type = 1
	return 1
end
