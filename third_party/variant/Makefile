MASON = .mason/mason
BOOST_VERSION = boost 1.60.0

CXX := $(CXX)
CXX_STD ?= c++11

BOOST_FLAGS = `$(MASON) cflags $(BOOST_VERSION)`
RELEASE_FLAGS = -O3 -DNDEBUG -march=native -DSINGLE_THREADED -fvisibility-inlines-hidden
DEBUG_FLAGS = -O0 -g -DDEBUG -fno-inline-functions
COMMON_FLAGS = -Wall -pedantic -Wextra -Wsign-compare -Wsign-conversion -Wshadow -Wunused-parameter -std=$(CXX_STD)
CXXFLAGS := $(CXXFLAGS)
LDFLAGS := $(LDFLAGS)

ALL_HEADERS = $(shell find include/mapbox/ '(' -name '*.hpp' ')')

all: out/bench-variant out/unique_ptr_test out/unique_ptr_test out/recursive_wrapper_test out/binary_visitor_test out/lambda_overload_test out/hashable_test

mason_packages:
	git submodule update --init .mason
	$(MASON) install $(BOOST_VERSION)

./deps/gyp:
	git clone --depth 1 https://chromium.googlesource.com/external/gyp.git ./deps/gyp

gyp: ./deps/gyp
	deps/gyp/gyp --depth=. -Goutput_dir=./ --generator-output=./out -f make
	make V=1 -C ./out tests
	./out/Release/tests

out/bench-variant-debug: Makefile mason_packages test/bench_variant.cpp
	mkdir -p ./out
	$(CXX) -o out/bench-variant-debug test/bench_variant.cpp -I./include -Itest/include -pthreads $(DEBUG_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/bench-variant: Makefile mason_packages test/bench_variant.cpp
	mkdir -p ./out
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./include -Itest/include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/unique_ptr_test: Makefile mason_packages test/unique_ptr_test.cpp
	mkdir -p ./out
	$(CXX) -o out/unique_ptr_test test/unique_ptr_test.cpp -I./include -Itest/include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/recursive_wrapper_test: Makefile mason_packages test/recursive_wrapper_test.cpp
	mkdir -p ./out
	$(CXX) -o out/recursive_wrapper_test test/recursive_wrapper_test.cpp -I./include -Itest/include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/binary_visitor_test: Makefile mason_packages test/binary_visitor_test.cpp
	mkdir -p ./out
	$(CXX) -o out/binary_visitor_test test/binary_visitor_test.cpp -I./include -Itest/include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/lambda_overload_test: Makefile mason_packages test/lambda_overload_test.cpp
	mkdir -p ./out
	$(CXX) -o out/lambda_overload_test test/lambda_overload_test.cpp -I./include -Itest/include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/hashable_test: Makefile mason_packages test/hashable_test.cpp
	mkdir -p ./out
	$(CXX) -o out/hashable_test test/hashable_test.cpp -I./include -Itest/include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

bench: out/bench-variant out/unique_ptr_test out/unique_ptr_test out/recursive_wrapper_test out/binary_visitor_test
	./out/bench-variant 100000
	./out/unique_ptr_test 100000
	./out/recursive_wrapper_test 100000
	./out/binary_visitor_test 100000

out/unit.o: Makefile test/unit.cpp
	mkdir -p ./out
	$(CXX) -c -o $@ test/unit.cpp -Itest/include $(DEBUG_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS)

out/%.o: test/t/%.cpp Makefile $(ALL_HEADERS)
	mkdir -p ./out
	$(CXX) -c -o $@ $< -Iinclude -Itest/include $(DEBUG_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS)

out/unit: out/unit.o out/binary_visitor_1.o out/binary_visitor_2.o out/binary_visitor_3.o out/binary_visitor_4.o out/binary_visitor_5.o out/binary_visitor_6.o out/issue21.o out/issue122.o out/mutating_visitor.o out/optional.o out/recursive_wrapper.o out/sizeof.o out/unary_visitor.o out/variant.o
	mkdir -p ./out
	$(CXX) -o $@ $^ $(LDFLAGS)

test: out/unit
	./out/unit

coverage:
	mkdir -p ./out
	$(CXX) -o out/cov-test --coverage test/unit.cpp test/t/*.cpp -I./include -Itest/include $(DEBUG_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS)

sizes: Makefile
	mkdir -p ./out
	@$(CXX) -o ./out/our_variant_hello_world.out include/mapbox/variant.hpp -I./include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) &&  du -h ./out/our_variant_hello_world.out
	@$(CXX) -o ./out/boost_variant_hello_world.out `$(MASON) prefix boost 1.60.0`/include/boost/variant.hpp -I./include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(BOOST_FLAGS) &&  du -h ./out/boost_variant_hello_world.out
	@$(CXX) -o ./out/our_variant_hello_world ./test/our_variant_hello_world.cpp -I./include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) &&  du -h ./out/our_variant_hello_world
	@$(CXX) -o ./out/boost_variant_hello_world ./test/boost_variant_hello_world.cpp -I./include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS)  $(BOOST_FLAGS) &&  du -h ./out/boost_variant_hello_world

profile: out/bench-variant-debug
	mkdir -p profiling/
	rm -rf profiling/*
	iprofiler -timeprofiler -d profiling/ ./out/bench-variant-debug 500000

clean:
	rm -rf ./out
	rm -rf *.dSYM
	rm -f unit.gc*
	rm -f *gcov
	rm -f test/unit.gc*
	rm -f test/*gcov
	rm -f *.gcda *.gcno

pgo: out Makefile
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS) -pg -fprofile-generate
	./test-variant 500000 >/dev/null 2>/dev/null
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./include $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS) -fprofile-use

.PHONY: sizes test
