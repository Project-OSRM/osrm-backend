-- Testbot profile

-- Moves at fixed, well-known speeds, practical for testing speed and travel times:

-- Primary road:	36km/h = 36000m/3600s = 100m/10s
-- Secondary road:	18km/h = 18000m/3600s = 100m/20s
-- Tertiary road:	12km/h = 12000m/3600s = 100m/30s

speed_profile = { 
	["primary"] = 36,
	["secondary"] = 18,
	["tertiary"] = 12,
	["default"] = 24
}

function node_function (node)
	local traffic_signal = node.tags:Find("highway")

	if traffic_signal == "traffic_signals" then
		node.traffic_light = true;
		-- TODO: a way to set the penalty value
	end
	return 1
end

function way_function (way, numberOfNodesInWay)
	-- A way must have two nodes or more
	if(numberOfNodesInWay < 2) then
		return 0;
	end
	
	local highway = way.tags:Find("highway")
	local name = way.tags:Find("name")
	way.name = name
	way.speed = speed_profile[highway] or speed_profile['default']
	way.direction = Way.bidirectional
	way.type = 1
	return 1
end
