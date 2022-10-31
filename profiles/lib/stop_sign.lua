local StopSign = {}

TrafficFlowControlNode = require("lib/traffic_flow_control_node")

function StopSign.get_value(node)
    return TrafficFlowControlNode.get_value(node, "stop")
end

return StopSign

