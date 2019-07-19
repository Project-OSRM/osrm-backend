package main

import (
	"testing"
	"sync"
)

func TestDumperStatistic(t *testing.T) {
	var d dumperStatistic
	var wg sync.WaitGroup

	wg.Add(10)
	d.Init(10)
	for i := 0; i < 10; i++ {
		go accumulateDumper(&d, &wg)
	}
	wg.Wait()
	d.Close()

	sum:= d.Sum()

	if (sum.wayCnt != 10) || (sum.nodeCnt != 20) || (sum.fwdRecordCnt != 30) || (sum.bwdRecordCnt != 40) || (sum.wayMatchedCnt != 50) || (sum.nodeMatchedCnt != 60) {
			t.Error("TestDumperStatistic failed.\n")
		}

}

func accumulateDumper(d *dumperStatistic, wg *sync.WaitGroup) {
	d.Update(1,2,3,4,5,6)
	wg.Done()
}

