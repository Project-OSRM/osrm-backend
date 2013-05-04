-- Foot profile

-- Begin of globals

bollards_whitelist = { [""] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true}
access_tag_whitelist = { ["yes"] = true, ["foot"] = true, ["permissive"] = true, ["designated"] = true  }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags = { "foot", "access" }
service_tag_restricted = { ["parking_aisle"] = true }
ignore_in_grid = { ["ferry"] = true }
restriction_exception_tags = { "foot" }

speed_profile = { 
    ["primary"] = 5,
    ["primary_link"] = 5,
    ["secondary"] = 5,
    ["secondary_link"] = 5,
    ["tertiary"] = 5,
    ["tertiary_link"] = 5,
    ["unclassified"] = 5,
    ["residential"] = 5,
    ["road"] = 5,
    ["living_street"] = 5,
    ["service"] = 5,
    ["track"] = 5,
    ["path"] = 5,
    ["steps"] = 5,
    ["ferry"] = 5,
    ["pedestrian"] = 5,
    ["footway"] = 5,
    ["pier"] = 5,
    ["default"] = 5
}


take_minimum_of_speeds 	= true
obey_oneway 			= true
obey_bollards 			= false
use_restrictions 		= false
ignore_areas 			= true -- future feature
traffic_signal_penalty 	= 2
u_turn_penalty 			= 2
use_turn_restrictions   = false
-- End of globals

function get_exceptions(vector)
    for i,v in ipairs(restriction_exception_tags) do 
        vector:Add(v)
    end
end

function node_function (node)
    local barrier = node.tags:Find ("barrier")
    local access = node.tags:Find ("access")
    local traffic_signal = node.tags:Find("highway")

    --flag node if it carries a traffic light

    if traffic_signal == "traffic_signals" then
        node.traffic_light = true;
    end

    if obey_bollards then
        --flag node as unpassable if it black listed as unpassable
        if access_tag_blacklist[barrier] then
            node.bollard = true;
        end

        --reverse the previous flag if there is an access tag specifying entrance
        if node.bollard and not bollards_whitelist[barrier] and not access_tag_whitelist[barrier] then
            node.bollard = false;
        end
    end
    return true
end

function way_function (way)

    -- First, get the properties of each way that we come across
    local highway = way.tags:Find("highway")
    local name = way.tags:Find("name")
    local ref = way.tags:Find("ref")
    local junction = way.tags:Find("junction")
    local route = way.tags:Find("route")
    local maxspeed = parseMaxspeed(way.tags:Find ( "maxspeed") )
    local man_made = way.tags:Find("man_made")
    local barrier = way.tags:Find("barrier")
    local oneway = way.tags:Find("oneway")
    local onewayClass = way.tags:Find("oneway:foot")
    local duration  = way.tags:Find("duration")
    local service  = way.tags:Find("service")
    local area = way.tags:Find("area")
    local access = way.tags:Find("access")

    -- Second parse the way according to these properties

    if ignore_areas and "yes"==area then
        return false
    end

    -- Check if we are allowed to access the way
    if access_tag_blacklist[access] and access_tag_blacklist[access] then
        return false
    end

    -- Check if our vehicle types are forbidden
    for i,v in ipairs(access_tags) do 
        local mode_value = way.tags:Find(v)
        if mode_value and "no"==mode_value then
            return 0;
        end
    end


    -- Set the name that will be used for instructions  
    if "" ~= ref then
        way.name = ref
    elseif "" ~= name then
        way.name = name
    end

    if "roundabout" == junction then
        way.roundabout = true;
    end

    -- Handling ferries and piers
    if (speed_profile[route] and speed_profile[route]>0) or (speed_profile[man_made] and speed_profile[man_made]>0) then
        way.forward.mode = 2
        way.backward.mode = 2
        if durationIsValid(duration) then
            way.duration = math.max( 1, parseDuration(duration) )
        else
            way.forward.speed = speed_profile[highway]
            way.backward.speed = speed_profile[highway]
        end
    else
        if speed_profile[route] then
            highway = route
        elseif speed_profile[man_made] then
            highway = man_made
        end
        if speed_profile[highway] then 
            way.forward.mode = 1
            way.backward.mode = 1
            way.forward.speed = speed_profile[highway]
            way.backward.speed = speed_profile[highway]
        end
        
        -- ignore oneway, but respect oneway:foot
        if onewayClass=="yes" or onewayClass=="1" or onewayClass=="true" then
            way.backward.mode = 0
        elseif onewayClass=="-1" then
            way.forward.mode = 0
        end
        
        -- restricted areas
        if access_tag_restricted[access] then
            way.is_access_restricted = true
        end
        if service_tag_restricted[service] then
            way.is_access_restricted = true
        end
    end
    
    return true
end
