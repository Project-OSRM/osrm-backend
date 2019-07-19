package main

import (
	"fmt"
)

type dumperStatisticItems struct {
	wayCnt               uint64
	nodeCnt              uint64
	fwdRecordCnt         uint64
	bwdRecordCnt         uint64
	wayMatchedCnt        uint64
	nodeMatchedCnt       uint64
	fwdTrafficMatchedCnt uint64
	bwdTrafficMatchedCnt uint64
}

type dumperStatistic struct {
	c     chan dumperStatisticItems
	sum   dumperStatisticItems
	init  bool
	close bool
}

func (d *dumperStatistic) Init(n int) {
	d.c = make(chan dumperStatisticItems, n)
	d.init = true
}

func (d *dumperStatistic) Close() {
	if !d.init {
		return
	}
	close(d.c)
	for item := range d.c {
		d.sum.wayCnt += item.wayCnt
		d.sum.nodeCnt += item.nodeCnt
		d.sum.fwdRecordCnt += item.fwdRecordCnt
		d.sum.bwdRecordCnt += item.bwdRecordCnt
		d.sum.wayMatchedCnt += item.wayMatchedCnt
		d.sum.nodeMatchedCnt += item.nodeMatchedCnt
		d.sum.fwdTrafficMatchedCnt += item.fwdTrafficMatchedCnt
		d.sum.bwdTrafficMatchedCnt += item.bwdTrafficMatchedCnt
	}
	d.close = true
}

func (d *dumperStatistic) Sum() dumperStatisticItems {
	return d.sum
}

func (d *dumperStatistic) Update(wayCnt uint64, nodeCnt uint64, fwdRecordCnt uint64,
	bwdRecordCnt uint64, wayMatchedCnt uint64, nodeMatchedCnt uint64,
	fwdTrafficMatchedCnt uint64, bwdTrafficMatchedCnt uint64) {
	if !d.init {
		fmt.Printf("dumperStatistic->Update() failed, please call Init() first otherwise will block all functions. \n")
		return
	}
	d.c <- (dumperStatisticItems{wayCnt, nodeCnt, fwdRecordCnt, bwdRecordCnt, wayMatchedCnt, nodeMatchedCnt, fwdTrafficMatchedCnt, bwdTrafficMatchedCnt})
}

func (d *dumperStatistic) Output() {
	if !d.close {
		fmt.Printf("Close() hasn't been called, no statistic collected.\n")
		return
	}

	fmt.Printf("Statistic: \n")
	fmt.Printf("Load %d way from data with %d nodes.\n", d.sum.wayCnt, d.sum.nodeCnt)
	fmt.Printf("%d way with %d nodes matched with traffic record.\n",
		d.sum.wayMatchedCnt, d.sum.nodeMatchedCnt)
	fmt.Printf("%d traffic records(%d forward and %d backward) have been matched.\n",
		d.sum.fwdTrafficMatchedCnt+d.sum.bwdTrafficMatchedCnt, d.sum.fwdTrafficMatchedCnt, d.sum.bwdTrafficMatchedCnt)
	fmt.Printf("Generate %d records in final result with %d of them from forward traffic and %d from backword.\n",
		d.sum.fwdRecordCnt+d.sum.bwdRecordCnt, d.sum.fwdRecordCnt, d.sum.bwdRecordCnt)
}
