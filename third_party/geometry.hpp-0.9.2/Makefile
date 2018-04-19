CXXFLAGS += -I include -std=c++14 -DDEBUG -O0 -Wall -Wextra -Werror
MASON ?= .mason/mason

VARIANT = 1.1.4

default: test

$(MASON):
	git submodule update --init

mason_packages/headers/variant/$(VARIANT):
	$(MASON) install variant $(VARIANT)

test: tests/* include/mapbox/geometry/* mason_packages/headers/variant/$(VARIANT) Makefile
	$(CXX) tests/*.cpp $(CXXFLAGS) `$(MASON) cflags variant $(VARIANT)` -o test
	./test

clean:
	rm -f test
