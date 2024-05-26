MASON = .mason/mason
BOOST_VERSION = 1.62.0

CXX := $(CXX)
CXX_STD ?= c++11

BOOST_ROOT = $(shell $(MASON) prefix boost $(BOOST_VERSION))
BOOST_FLAGS = -isystem $(BOOST_ROOT)/include/
RELEASE_FLAGS = -O3 -DNDEBUG -march=native -DSINGLE_THREADED -fvisibility-inlines-hidden -fvisibility=hidden
DEBUG_FLAGS = -O0 -g -DDEBUG -fno-inline-functions -fno-omit-frame-pointer -fPIE
WARNING_FLAGS = -Werror -Wall -Wextra -pedantic \
		-Wformat=2 -Wsign-conversion -Wshadow -Wunused-parameter

COMMON_FLAGS = -std=$(CXX_STD)
COMMON_FLAGS += $(WARNING_FLAGS)

CXXFLAGS := $(CXXFLAGS)
LDFLAGS := $(LDFLAGS)

export BUILDTYPE ?= Release

OS := $(shell uname -s)
ifeq ($(OS), Linux)
  EXTRA_FLAGS = -pthread
endif
ifeq ($(OS), Darwin)
  OSX_OLDEST_SUPPORTED ?= 10.7
  # we need to explicitly ask for libc++ otherwise the
  # default will flip back to libstdc++ for mmacosx-version-min < 10.9
  EXTRA_FLAGS = -stdlib=libc++ -mmacosx-version-min=$(OSX_OLDEST_SUPPORTED)
endif


ifeq ($(BUILDTYPE),Release)
	FINAL_CXXFLAGS := $(COMMON_FLAGS) $(RELEASE_FLAGS) $(CXXFLAGS) $(EXTRA_FLAGS)
else
	FINAL_CXXFLAGS := $(COMMON_FLAGS) $(DEBUG_FLAGS) $(CXXFLAGS) $(EXTRA_FLAGS)
endif



ALL_HEADERS = $(shell find include/mapbox/ '(' -name '*.hpp' ')')

all: out/bench-variant out/unique_ptr_test out/unique_ptr_test out/recursive_wrapper_test out/binary_visitor_test out/lambda_overload_test out/hashable_test

$(MASON):
	git submodule update --init .mason

mason_packages/headers/boost: $(MASON)
	$(MASON) install boost $(BOOST_VERSION)

./deps/gyp:
	git clone --depth 1 https://chromium.googlesource.com/external/gyp.git ./deps/gyp

gyp: ./deps/gyp
	deps/gyp/gyp --depth=. -Goutput_dir=./ --generator-output=./out -f make
	make V=1 -C ./out tests
	./out/$(BUILDTYPE)/tests

out/bench-variant-debug: Makefile mason_packages/headers/boost test/bench_variant.cpp
	mkdir -p ./out
	$(CXX) -o out/bench-variant-debug test/bench_variant.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/bench-variant: Makefile mason_packages/headers/boost test/bench_variant.cpp
	mkdir -p ./out
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/unique_ptr_test: Makefile mason_packages/headers/boost test/unique_ptr_test.cpp
	mkdir -p ./out
	$(CXX) -o out/unique_ptr_test test/unique_ptr_test.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/recursive_wrapper_test: Makefile mason_packages/headers/boost test/recursive_wrapper_test.cpp
	mkdir -p ./out
	$(CXX) -o out/recursive_wrapper_test test/recursive_wrapper_test.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/binary_visitor_test: Makefile mason_packages/headers/boost test/binary_visitor_test.cpp
	mkdir -p ./out
	$(CXX) -o out/binary_visitor_test test/binary_visitor_test.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/lambda_overload_test: Makefile mason_packages/headers/boost test/lambda_overload_test.cpp
	mkdir -p ./out
	$(CXX) -o out/lambda_overload_test test/lambda_overload_test.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

out/hashable_test: Makefile mason_packages/headers/boost test/hashable_test.cpp
	mkdir -p ./out
	$(CXX) -o out/hashable_test test/hashable_test.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS)

bench: out/bench-variant out/unique_ptr_test out/unique_ptr_test out/recursive_wrapper_test out/binary_visitor_test
	./out/bench-variant 100000
	./out/unique_ptr_test 100000
	./out/recursive_wrapper_test 100000
	./out/binary_visitor_test 100000

out/unit.o: Makefile test/unit.cpp
	mkdir -p ./out
	$(CXX) -c -o $@ test/unit.cpp -isystem test/include $(FINAL_CXXFLAGS)

out/%.o: test/t/%.cpp Makefile $(ALL_HEADERS)
	mkdir -p ./out
	$(CXX) -c -o $@ $< -Iinclude -isystem test/include $(FINAL_CXXFLAGS)

out/unit: out/unit.o \
          out/binary_visitor_1.o \
          out/binary_visitor_2.o \
          out/binary_visitor_3.o \
          out/binary_visitor_4.o \
          out/binary_visitor_5.o \
          out/binary_visitor_6.o \
          out/issue21.o \
          out/issue122.o \
          out/mutating_visitor.o \
          out/optional.o \
          out/recursive_wrapper.o \
          out/sizeof.o \
          out/unary_visitor.o \
          out/variant.o \
          out/variant_alternative.o \
          out/nothrow_move.o \
          out/visitor_result_type.o \

	mkdir -p ./out
	$(CXX) -o $@ $^ $(LDFLAGS)

test: out/unit
	./out/unit

coverage:
	mkdir -p ./out
	$(CXX) -o out/cov-test --coverage test/unit.cpp test/t/*.cpp -I./include -isystem test/include $(FINAL_CXXFLAGS) $(LDFLAGS)

sizes: Makefile
	mkdir -p ./out
	@$(CXX) -o ./out/our_variant_hello_world.out include/mapbox/variant.hpp -I./include $(FINAL_CXXFLAGS) &&  ls -lah ./out/our_variant_hello_world.out
	@$(CXX) -o ./out/boost_variant_hello_world.out $(BOOST_ROOT)/include/boost/variant.hpp -I./include $(FINAL_CXXFLAGS) $(BOOST_FLAGS) &&  ls -lah ./out/boost_variant_hello_world.out
	@$(CXX) -o ./out/our_variant_hello_world ./test/our_variant_hello_world.cpp -I./include $(FINAL_CXXFLAGS) &&  ls -lah ./out/our_variant_hello_world
	@$(CXX) -o ./out/boost_variant_hello_world ./test/boost_variant_hello_world.cpp -I./include $(FINAL_CXXFLAGS) $(BOOST_FLAGS) &&  ls -lah ./out/boost_variant_hello_world

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
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS) -pg -fprofile-generate
	./test-variant 500000 >/dev/null 2>/dev/null
	$(CXX) -o out/bench-variant test/bench_variant.cpp -I./include $(FINAL_CXXFLAGS) $(LDFLAGS) $(BOOST_FLAGS) -fprofile-use

.PHONY: sizes test
