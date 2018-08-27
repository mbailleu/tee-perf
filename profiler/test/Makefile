SRCDIR=..

.PHONY: all ctest cxxtest multithreaded clean

all: ctest cxxtest multithreaded

ctest:
	make -C C

cxxtest:
	make -C CXX

multithreaded:
	make -C multithreaded

clean:
	-make -C C clean
	-make -C CXX clean
	-make -C multithreaded clean
