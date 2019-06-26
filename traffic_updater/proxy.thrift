
struct Flow {
    1: optional i64 fromId;
    2: optional i64 toId;
    3: required i64 wayId;
    4: required double speed;
    5: required i32 trafficLevel;
}

service ProxyService {
    list<Flow> getAllFlows()
    Flow getFlowById(1:i64 wayId)
    list<Flow> getFlowsByIds(1:list<i64> wayIds)
}
