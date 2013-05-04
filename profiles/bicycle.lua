require("lib/access")

-- Begin of globals
barrier_whitelist = { [""] = true, ["cycle_barrier"] = true, ["bollard"] = true, ["entrance"] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["no"] = true}
access_tag_whitelist = { ["yes"] = true, ["permissive"] = true, ["designated"] = true	}
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierachy = { "bicycle", "vehicle", "access" }
cycleway_tags = {["track"]=true,["lane"]=true,["opposite"]=true,["opposite_lane"]=true,["opposite_track"]=true,["share_busway"]=true,["sharrow"]=true,["shared"]=true }
service_tag_restricted = { ["parking_aisle"] = true }
restriction_exception_tags = { "bicycle", "vehicle", "access" }

default_speed = 15

walking_speed = 6

bicycle_speeds = { 
	["cycleway"] = default_speed,
	["primary"] = default_speed,
	["primary_link"] = default_speed,
	["secondary"] = default_speed,
	["secondary_link"] = default_speed,
	["tertiary"] = default_speed,
	["tertiary_link"] = default_speed,
	["residential"] = default_speed,
	["unclassified"] = default_speed,
	["living_street"] = default_speed,
	["road"] = default_speed,
	["service"] = default_speed,
	["track"] = 12,
	["path"] = 12
	--["footway"] = 12,
	--["pedestrian"] = 12,
}

pedestrian_speeds = { 
	["footway"] = walking_speed,
	["pedestrian"] = walking_speed,
	["steps"] = 2
}

railway_speeds = { 
	["train"] = 10,
	["railway"] = 10,
	["subway"] = 10,
	["light_rail"] = 10,
	["monorail"] = 10,
	["tram"] = 10
}

platform_speeds = { 
	["platform"] = walking_speed
}

amenity_speeds = { 
	["parking"] = 10,
	["parking_entrance"] = 10
}

man_made_speeds = { 
	["pier"] = walking_speed
}

route_speeds = { 
	["ferry"] = 5
}

surface_speeds = { 
	["cobblestone:flattened"] = 10,
	["paving_stones"] = 10,
	["compacted"] = 10,
	["cobblestone"] = 6,
	["unpaved"] = 6,
	["fine_gravel"] = 6,
	["gravel"] = 6,
	["fine_gravel"] = 6,
	["pebbelstone"] = 6,
	["ground"] = 6,
	["dirt"] = 6,
	["earth"] = 6,
	["grass"] = 6,
	["mud"] = 3,
	["sand"] = 3	
}

take_minimum_of_speeds 	= true
obey_oneway 			= true
obey_bollards 			= false
use_restrictions 		= true
ignore_areas 			= true -- future feature
traffic_signal_penalty 	= 5
u_turn_penalty 			= 20
use_turn_restrictions   = false
turn_penalty 			= 60
turn_bias               = 1.4
-- End of globals

function get_exceptions(vector)
	for i,v in ipairs(restriction_exception_tags) do 
		vector:Add(v)
	end
end

function node_function (node)
	local barrier = node.tags:Find ("barrier")
	local access = Access.find_access_tag(node, access_tags_hierachy)
	local traffic_signal = node.tags:Find("highway")
	
	-- flag node if it carries a traffic light	
	if traffic_signal == "traffic_signals" then
		node.traffic_light = true
	end
	
	-- parse access and barrier tags
	if access and access ~= "" then
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
	
	return true
end

function way_function (way)
	-- initial routability check, filters out buildings, boundaries, etc
	local highway = way.tags:Find("highway")
	local route = way.tags:Find("route")
	local man_made = way.tags:Find("man_made")
	local railway = way.tags:Find("railway")
	local amenity = way.tags:Find("amenity")
	local public_transport = way.tags:Find("public_transport")
    if (not highway or highway == '') and 
		(not route or route == '') and 
		(not railway or railway=='') and 
		(not amenity or amenity=='') and
		(not man_made or man_made=='') and
    	(not public_transport or public_transport=='')
    	then
    	return false
    end
    
    -- don't route on ways or railways that are still under construction
    if highway=='construction' or railway=='construction' then
        return false
    end
    
	-- access
 	local access = Access.find_access_tag(way, access_tags_hierachy)
    if access_tag_blacklist[access] then
		return false
    end


	-- other tags
	local name = way.tags:Find("name")
	local ref = way.tags:Find("ref")
	local junction = way.tags:Find("junction")
	local maxspeed = parseMaxspeed(way.tags:Find ( "maxspeed") )
	local maxspeed_forward = parseMaxspeed(way.tags:Find( "maxspeed:forward"))
	local maxspeed_backward = parseMaxspeed(way.tags:Find( "maxspeed:backward"))
	local barrier = way.tags:Find("barrier")
	local oneway = way.tags:Find("oneway")
	local onewayClass = way.tags:Find("oneway:bicycle")
	local cycleway = way.tags:Find("cycleway")
	local cycleway_left = way.tags:Find("cycleway:left")
	local cycleway_right = way.tags:Find("cycleway:right")
	local duration	= way.tags:Find("duration")
	local service	= way.tags:Find("service")
	local area = way.tags:Find("area")
	local foot = way.tags:Find("foot")
	local surface = way.tags:Find("surface")
	local foot_forward = way.tags:Find("foot:forward")
	local foot_backward = way.tags:Find("foot:backward")

	-- name	
	if "" ~= ref and "" ~= name then
		way.name = name .. ' / ' .. ref
    elseif "" ~= ref then
    	way.name = ref
	elseif "" ~= name then
		way.name = name
	else
		way.name = "{highway:"..highway.."}"	-- if no name exists, use way type
		                                        -- this encoding scheme is excepted to be a temporary solution
	end
	
	way.forward.mode = 1
	way.backward.mode = 1
	
	-- speed
    if route_speeds[route] then
		-- ferries (doesn't cover routes tagged using relations)
		way.ignore_in_grid = true
		if durationIsValid(duration) then
			way.duration = math.max( 1, parseDuration(duration) )
		else
		 	way.forward.speed = route_speeds[route]
		 	way.backward.speed = route_speeds[route]
		end
    	way.forward.mode = 2
    	way.backward.mode = 2
	elseif platform_speeds[railway] then
		-- railway platforms (old tagging scheme)
		way.forward.speed = platform_speeds[railway]
		way.backward.speed = platform_speeds[railway]
	elseif platform_speeds[public_transport] then
		-- public_transport platforms (new tagging platform)
		way.forward.speed = platform_speeds[public_transport]
		way.backward.speed = platform_speeds[public_transport]
    elseif railway_speeds[railway] then
	 	-- railways
		if access and access_tag_whitelist[access] then
			way.forward.speed = railway_speeds[railway]		
			way.backward.speed = railway_speeds[railway]		
		end
	elseif amenity_speeds[amenity] then
		-- parking areas
		way.forward.speed = amenity_speeds[amenity]
		way.backward.speed = amenity_speeds[amenity]
	elseif bicycle_speeds[highway] then
		-- regular ways
      	way.forward.speed = bicycle_speeds[highway]
      	way.backward.speed = bicycle_speeds[highway]
	elseif access_tag_whitelist[access] then
	    -- unknown way, but valid access tag
		way.forward.speed = default_speed
		way.backward.speed = default_speed
	else
	    -- biking not allowed, maybe we can push our bike?
	    -- essentially requires pedestrian profiling, for example foot=no mean we can't push a bike
        if foot ~= 'no' then
	        if pedestrian_speeds[highway] then
	            -- pedestrian-only ways and areas
        		way.forward.speed = pedestrian_speeds[highway]
        		way.backward.speed = pedestrian_speeds[highway]
            	way.forward.mode = 3
            	way.backward.mode = 3
        	elseif man_made and man_made_speeds[man_made] then
            	-- man made structures
            	way.forward.speed = man_made_speeds[man_made]
            	way.backward.speed = man_made_speeds[man_made]
            	way.forward.mode = 3
            	way.backward.mode = 3
            elseif foot == 'yes' then
                way.forward.speed = walking_speed
                way.backward.speed = walking_speed
            	way.forward.mode = 3
            	way.backward.mode = 3
            elseif foot_forward == 'yes' then
                way.forward.speed = walking_speed
            	way.forward.mode = 3
            	way.backward.mode = 0
            elseif foot_backward == 'yes' then
                way.backward.speed = walking_speed
            	way.backward.mode = 3
            	way.forward.mode = 0
            end
        end
    end
		
	-- direction
	local impliedOneway = false
	if junction == "roundabout" or highway == "motorway_link" or highway == "motorway" then
		impliedOneway = true
	end
	
	if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
    	way.backward.mode = 0
	elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" then
	    -- prevent implied oneway
	elseif onewayClass == "-1" then
    	way.forward.mode = 0
	elseif oneway == "no" or oneway == "0" or oneway == "false" then
	    -- prevent implied oneway
	elseif string.find(cycleway, "opposite") == 1 then
		if impliedOneway then
        	way.forward.mode = 0
        	way.backward.mode = 1
		end
	elseif cycleway_tags[cycleway_left] and cycleway_tags[cycleway_right] then
	    -- prevent implied
	elseif cycleway_tags[cycleway_left] then
		if impliedOneway then
        	way.forward.mode = 0
        	way.backward.mode = 1
		end
	elseif cycleway_tags[cycleway_right] then
		if impliedOneway then
        	way.forward.mode = 1
        	way.backward.mode = 0
		end
	elseif oneway == "-1" then
		way.forward.mode = 0
	elseif oneway == "yes" or oneway == "1" or oneway == "true" or impliedOneway then
	    way.backward.mode = 0
    end
	
    
	-- pushing bikes
	if bicycle_speeds[highway] or pedestrian_speeds[highway] then
	    if foot ~= "no" then
	        if junction ~= "roundabout" then
            	if way.backward.mode == 0 then
            	    way.backward.speed = walking_speed
                	way.backward.mode = 3
                elseif way.forward.mode == 0 then
                    way.forward.speed = walking_speed
                	way.forward.mode = 3
            	end
            end
        end
    end
	
	-- cycleway speed
	if cycleway_tags[cycleway_right] then
		way.forward.speed = bicycle_speeds["cycleway"]
	elseif cycleway_tags[cycleway] then
		way.forward.speed = bicycle_speeds["cycleway"]
    end
	if cycleway_tags[cycleway_left] then
		way.backward.speed = bicycle_speeds["cycleway"]
    elseif cycleway_tags[cycleway] then
		way.backward.speed = bicycle_speeds["cycleway"]
	end
    
    -- surfaces
    if surface then
        surface_speed = surface_speeds[surface]
        if surface_speed then
            way.forward.speed = math.min(way.forward.speed, surface_speed)
            way.backward.speed  = math.min(way.backward.speed, surface_speed)
        end
    end

	-- maxspeed
    if maxspeed_forward and maxspeed_forward>0 then
        way.forward.speed = math.min(way.forward.speed, maxspeed_forward)
	elseif maxspeed and maxspeed>0 then
		way.forward.speed = math.min(way.forward.speed, maxspeed)
	end
	
    if maxspeed_backward and maxspeed_backward>0 then
        way.backward.speed = math.min(way.backward.speed, maxspeed_backward)
	elseif maxspeed and maxspeed>0 then
		way.backward.speed = math.min(way.backward.speed, maxspeed)
	end
		
	return true
end

function turn_function (angle)
    -- compute turn penalty as angle^2, with a left/right bias
    k = turn_penalty/(90.0*90.0)
	if angle>=0 then
	    return angle*angle*k/turn_bias
	else
	    return angle*angle*k*turn_bias
    end
end
