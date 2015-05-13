CXX := $(CXX)
BOOST_LIBS = -lboost_timer -lboost_system -lboost_chrono
RELEASE_FLAGS = -O3 -DNDEBUG -finline-functions -march=native -DSINGLE_THREADED
DEBUG_FLAGS = -O0 -g -DDEBUG -fno-inline-functions
COMMON_FLAGS = -Wall -Wsign-compare -Wsign-conversion -Wshadow -Wunused-parameter -pedantic -fvisibility-inlines-hidden -std=c++11
CXXFLAGS := $(CXXFLAGS)
LDFLAGS := $(LDFLAGS)

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
	CXXFLAGS += -stdlib=libc++
	LDFLAGS += -stdlib=libc++ -F/ -framework CoreFoundation
else
    BOOST_LIBS += -lrt
endif

ifeq (sizes,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
  ifndef RUN_ARGS
  $(error sizes target requires you pass full path to boost variant.hpp)
  endif
  .PHONY: $(RUN_ARGS)
endif

all: out/bench-variant out/unique_ptr_test out/unique_ptr_test out/recursive_wrapper_test out/binary_visitor_test

./deps/gyp:
	git clone --depth 1 https://chromium.googlesource.com/external/gyp.git ./deps/gyp

gyp: ./deps/gyp
	deps/gyp/gyp --depth=. -Goutput_dir=./ --generator-output=./out -f make
	make V=1 -C ./out tests
	./out/Release/tests

out/bench-variant-debug: Makefile test/bench_variant.cpp variant.hpp
	mkdir -p ./out
	$(CXX) -o out/bench-variant-debug test/bench_variant.cpp -I./ $(DEBUG_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS)

out/bench-variant: Makefile test/bench_variant.cpp variant.hpp
	mkdir -p ./out
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS)

out/unique_ptr_test: Makefile test/unique_ptr_test.cpp variant.hpp
	mkdir -p ./out
	$(CXX) -o out/unique_ptr_test test/unique_ptr_test.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS)

out/recursive_wrapper_test: Makefile test/recursive_wrapper_test.cpp variant.hpp
	mkdir -p ./out
	$(CXX) -o out/recursive_wrapper_test test/recursive_wrapper_test.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS)

out/binary_visitor_test: Makefile test/binary_visitor_test.cpp variant.hpp
	mkdir -p ./out
	$(CXX) -o out/binary_visitor_test test/binary_visitor_test.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS)

bench: out/bench-variant out/unique_ptr_test out/unique_ptr_test out/recursive_wrapper_test out/binary_visitor_test
	./out/bench-variant 100000
	./out/unique_ptr_test 100000
	./out/recursive_wrapper_test 100000
	./out/binary_visitor_test 100000

out/unit: Makefile test/unit.cpp test/optional_unit.cpp optional.hpp variant.hpp
	mkdir -p ./out
	$(CXX) -o out/unit test/unit.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS)
	$(CXX) -o out/optional_unit test/optional_unit.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS)

test: out/unit
	./out/unit
	./out/optional_unit

coverage:
	mkdir -p ./out
	$(CXX) -o out/cov-test --coverage test/unit.cpp -I./ $(DEBUG_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS)

sizes: Makefile variant.hpp
	mkdir -p ./out
	@$(CXX) -o ./out/variant_hello_world.out variant.hpp $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) &&  du -h ./out/variant_hello_world.out
	@$(CXX) -o ./out/boost_variant_hello_world.out $(RUN_ARGS) $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) &&  du -h ./out/boost_variant_hello_world.out
	@$(CXX) -o ./out/variant_hello_world ./test/variant_hello_world.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) &&  du -h ./out/variant_hello_world
	@$(CXX) -o ./out/boost_variant_hello_world ./test/boost_variant_hello_world.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) &&  du -h ./out/boost_variant_hello_world

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

pgo: out Makefile variant.hpp
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS) -pg -fprofile-generate
	./test-variant 500000 >/dev/null 2>/dev/null
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./ $(RELEASE_FLAGS) $(COMMON_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS) -fprofile-use

.PHONY: sizes test
