bollards_whitelist = { [""] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["no"] = true}
access_whitelist = { ["yes"] = true, ["motorcar"] = true, ["permissive"] = true }
access_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }

function node_function (node)
  local barrier = node.tags:Find("barrier")
  local access = node.tags:Find("access")
  local traffic_signal = node.tags:Find("highway")
  
  if traffic_signal == "traffic_signals" then
	node.traffic_light = true;
  end
  
  if access_blacklist[barrier] then
	node.bollard = true;
  end
  
  if not bollards_whitelist[barrier] then
	node.bollard = true;
  end
  return 1
end

function way_function (way)

  return 1
end
