# compiler and standards
CPP = g++ -Wall -Wextra

# message digest library dependencies, flags, and linking
MDDEPS = src/libmd/libmd.a src/libmd/md5.h
MDFLAGS = -L src/libmd/
MDLIBS = -lmd

# compilations
run: build/tracker
	./build/tracker

build/tracker: src/tracker.cpp src/tracker/tracker.hpp src/mix/util.hpp ${MDDEPS}
	${CPP} ${MDFLAGS} -o build/tracker $< ${MDLIBS}


# libraries (just for listing dependencies, does not compile anything)
libtracker: src/tracker/tracker.hpp #libutil
#libutil: src/mix/util.hpp libmd


# mix
.PHONY: clean
clean:
	rm -rf build/*
