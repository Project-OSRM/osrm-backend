package main

import (
	"testing"
	"reflect"
	"sync"
)


func TestLoadWay2Nodeids(t *testing.T) {
	// load result into sources
	var sources [TASKNUM]chan string
	for i := range sources {
		//fmt.Printf("&&& current i is %d\n", i)
		sources[i] = make(chan string, 10000)
	}
	go loadWay2NodeidsTable("./testdata/id-mapping.csv.snappy", sources)

	allWay2NodesChan := make(chan string, 10000)
	var wgs sync.WaitGroup
	wgs.Add(TASKNUM)
	for i:= 0; i < TASKNUM; i++ {
		//fmt.Printf("### current i is %d\n", i)
		go mergeChannels(sources[i], allWay2NodesChan, &wgs)
	}
	wgs.Wait()

	// dump result into map
	way2nodeids := make(map[string]bool)
	var wg sync.WaitGroup
	wg.Add(1)
	go func() {
		for elem := range allWay2NodesChan {
			way2nodeids[elem] = true
		}
		wg.Done()
	}()
	close(allWay2NodesChan)
	wg.Wait()

	// test map result
	way2nodeidsExpect := make(map[string]bool)
	generateMockWay2nodeids(way2nodeidsExpect)

	eq := reflect.DeepEqual(way2nodeids, way2nodeidsExpect)
	if !eq {
		t.Error("TestLoadWay2Nodeids failed to generate correct map\n")
	}
}

func mergeChannels(f <-chan string, t chan<- string, w *sync.WaitGroup) {
	for elem := range f {
		t <- elem
	}
	w.Done()
}

func generateMockWay2nodeids(way2nodeids map[string]bool) {
	way2nodeids["24418325,84760891102,19496208102"] = true
	way2nodeids["24418332,84762609102,244183320001101,84762607102"] = true
	way2nodeids["24418343,84760849102,84760850102"] = true
	way2nodeids["24418344,84760846102,84760858102"] = true
}

