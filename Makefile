all: 
	g++ -O3 -Wall -Wno-deprecated -fopenmp -march=native -lboost_regex-mt -lboost_iostreams-mt -lboost_thread-mt -lboost_system-mt -o routed routed.cpp -DNDEBUG
	g++ createHierarchy.cpp -fopenmp -Wno-deprecated -o createHierarchy -O3 -march=native -DNDEBUG
	g++ extractNetwork.cpp -fopenmp -Wno-deprecated -o extractNetwork -O3 -march=native -I/usr/include/libxml2/ -lxml2 -DNDEBUG
	g++ extractLargeNetwork.cpp -fopenmp -Wno-deprecated -o extractLargeNetwork -O3 -march=native -I/usr/include/libxml2/ -lxml2 -DNDEBUG -I/usr/include/include -lstxxl
clean:
	rm -rf createHierarchy routed extractNetwork *.o *.gch
