local GiveWay = {}

TrafficFlowControlNode = require("lib/traffic_flow_control_node")

function GiveWay.get_value(node)
    return TrafficFlowControlNode.get_value(node, "give_way")
end

return GiveWay
