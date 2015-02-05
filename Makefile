
all:
	mkdir -p build && cd build && cmake .. && $(MAKE)

doc:
	mkdir -p build && cd build && cmake .. && $(MAKE) doc

clean:
	if test -d build; then cd build && $(MAKE) clean; fi

distclean:
	rm -fr build

#deb:
#	debuild -I -us -uc
#
#deb-clean:
#	debuild clean

indent:
	astyle --style=java --indent-namespaces --indent-switches --pad-header --lineend=linux --suffix=none --recursive include/\*.hpp examples/\*.cpp test/\*.cpp
#	astyle --style=java --indent-namespaces --indent-switches --pad-header --unpad-paren --align-pointer=type --lineend=linux --suffix=none --recursive include/\*.hpp examples/\*.cpp test/\*.cpp

.PHONY: clean distclean deb deb-clean doc indent

