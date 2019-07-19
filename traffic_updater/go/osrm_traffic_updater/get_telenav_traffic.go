package main

import (
	"context"
	"fmt"
	"strconv"
	"time"

	"github.com/Telenav/osrm-backend/traffic_updater/go/gen-go/proxy"
	"github.com/apache/thrift/lib/go/thrift"
)

func getTrafficFlow(ip string, port int, m map[int64]int, c chan<- bool) {
	var transport thrift.TTransport
	var err error

	startTime := time.Now()

	// make socket
	targetServer := ip + ":" + strconv.Itoa(port)
	fmt.Println("connect traffic proxy " + targetServer)
	transport, err = thrift.NewTSocket(targetServer)
	if err != nil {
		fmt.Println("Error opening socket:", err)
		c <- false
		return
	}

	// Buffering
	transport, err = thrift.NewTFramedTransportFactoryMaxLength(thrift.NewTTransportFactory(), 1024*1024*1024).GetTransport(transport)
	if err != nil {
		fmt.Println("Error get transport:", err)
		c <- false
		return
	}
	defer transport.Close()
	if err := transport.Open(); err != nil {
		fmt.Println("Error opening transport:", err)
		c <- false
		return
	}

	// protocol encoder&decoder
	protocol := thrift.NewTCompactProtocolFactory().GetProtocol(transport)

	// create proxy client
	client := proxy.NewProxyServiceClient(thrift.NewTStandardClient(protocol, protocol))

	// get flows
	fmt.Println("getting flows")
	var defaultCtx = context.Background()
	var flows []*proxy.Flow
	flows, err = client.GetAllFlows(defaultCtx)
	if err != nil {
		fmt.Println("get flows failed:", err)
		c <- false
		return
	}
	fmt.Printf("got flows count: %d\n", len(flows))

	endTime := time.Now()
	fmt.Printf("Processing time for getting traffic flow takes %f seconds\n", endTime.Sub(startTime).Seconds())

	flows2map(flows, m)
	endTime2 := time.Now()
	fmt.Printf("Processing time for building traffic map takes %f seconds\n", endTime2.Sub(endTime).Seconds())

	c <- true
	return
}

func flows2map(flows []*proxy.Flow, m map[int64]int) {
	var fwdCnt, bwdCnt uint64
	for _, flow := range flows {
		wayid := flow.WayId
		m[wayid] = int(flow.Speed)

		if wayid > 0 {
			fwdCnt++
		} else {
			bwdCnt++
		}
	}

	fmt.Printf("Load map[wayid] to speed with %d items, %d forward and %d backward.\n", (fwdCnt+bwdCnt), fwdCnt, bwdCnt)
}
