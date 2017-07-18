CXXFLAGS     = -O2 -std=c++11 -I..
CXXFLAGS    += -Wall -pedantic -Wextra -D_XOPEN_SOURCE=700 
SPP_DEPS_1   =  spp.h spp_utils.h spp_dlalloc.h spp_traits.h spp_config.h
SPP_DEPS     = $(addprefix ../sparsepp/,$(SPP_DEPS_1))
TARGETS      = spp_test spp_alloc_test spp_bitset_test perftest1 bench


ifeq ($(OS),Windows_NT)
    LDFLAGS  = -lpsapi
endif

def: spp_test 

all: $(TARGETS)

clean:
	rm -rf $(TARGETS) vsprojects/x64/* vsprojects/x86/*

test:
	./spp_test

spp_test: spp_test.cc $(SPP_DEPS) makefile
	$(CXX) $(CXXFLAGS) -D_CRT_SECURE_NO_WARNINGS spp_test.cc -o spp_test

%: %.cc $(SPP_DEPS) makefile
	$(CXX) $(CXXFLAGS) -DNDEBUG $< -o $@ $(LDFLAGS)

