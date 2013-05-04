-- Testbot profile

-- Moves at fixed, well-known speeds, practical for testing speed and travel times:

-- Primary road:	36km/h = 36000m/3600s = 100m/10s
-- Secondary road:	18km/h = 18000m/3600s = 100m/20s
-- Tertiary road:	12km/h = 12000m/3600s = 100m/30s

-- modes:
-- 1: normal
-- 2: route
-- 3: river downstream
-- 4: river upstream
-- 5: steps down
-- 6: steps up

speed_profile = { 
	["primary"] = 36,
	["secondary"] = 18,
	["tertiary"] = 12,
	["steps"] = 6,
	["default"] = 24
}

-- these settings are read directly by osrm

take_minimum_of_speeds 	= true
obey_oneway 			= true
obey_bollards 			= true
use_restrictions 		= true
ignore_areas 			= true	-- future feature
traffic_signal_penalty 	= 7		-- seconds
u_turn_penalty 			= 20

modes = { "bot", "ferry", "downstream", "upstream" }

function get_modes(vector)
	for i,v in ipairs(modes) do 
		vector:Add(v)
	end
end

function limit_speed(speed, limits)
    -- don't use ipairs(), since it stops at the first nil value
    for i=1, #limits do
        limit = limits[i]
        if limit ~= nil and limit > 0 then
            if limit < speed then
                return limit        -- stop at first speedlimit that's smaller than speed
            end
        end
    end
    return speed
end

function node_function (node)
	local traffic_signal = node.tags:Find("highway")

	if traffic_signal == "traffic_signals" then
		node.traffic_light = true;
		-- TODO: a way to set the penalty value
	end
	return 1
end

function way_function (way)
	local highway = way.tags:Find("highway")
	local name = way.tags:Find("name")
	local oneway = way.tags:Find("oneway")
	local route = way.tags:Find("route")
	local duration = way.tags:Find("duration")
    local maxspeed = tonumber(way.tags:Find ( "maxspeed"))
    local maxspeed_forward = tonumber(way.tags:Find( "maxspeed:forward"))
    local maxspeed_backward = tonumber(way.tags:Find( "maxspeed:backward"))
	
	way.name = name

  	if route ~= nil and durationIsValid(duration) then
		way.duration = math.max( 1, parseDuration(duration) )
    	way.forward.mode = 2
    	way.backward.mode = 2
	else
	    local speed = speed_profile[highway] or speed_profile['default']

    	if highway == "river" then
        	way.forward.mode = 3
        	way.backward.mode = 4
    		way.forward.speed = speed*1.5
    		way.backward.speed = speed/1.5
        else
        	if highway == "steps" then
            	way.forward.mode = 5
            	way.backward.mode = 6
       	    else
        		way.forward.mode = 1
            	way.backward.mode = 1
        	end
            way.forward.speed = speed
    		way.backward.speed = speed
   	    end
            	
        if maxspeed_forward ~= nil and maxspeed_forward > 0 then
			way.forward.speed = maxspeed_forward
		else
			if maxspeed ~= nil and maxspeed > 0 and way.forward.speed > maxspeed then
				way.forward.speed = maxspeed
			end
		end
		
		if maxspeed_backward ~= nil and maxspeed_backward > 0 then
			way.backward.speed = maxspeed_backward
		else
			if maxspeed ~=nil and maxspeed > 0 and way.backward.speed > maxspeed then
				way.backward.speed = maxspeed
			end
		end  
	end
	
	if oneway == "-1" then
		way.forward.mode = 0
	elseif oneway == "yes" or oneway == "1" or oneway == "true" then
		way.backward.mode = 0
	end
	
	return 1
end
