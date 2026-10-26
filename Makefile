# compiler and standards
CPP = g++ -g # -Wall -Wextra

# message digest library dependencies, flags, and linking
MD_DEPS = src/libmd/libmd.a src/libmd/md5.h
MD_FLAGS = -L src/libmd/
MD_LIBS = -lmd

# libutil, depends on libmd (external)
UTIL_DEPS = src/mix/util.hpp ${MD_DEPS} 
UTIL_FLAGS = ${MD_FLAGS}
UTIL_LIBS = ${MD_LIBS}

# libtracker, depends on libutil (local)
TRACKER_DEPS = src/tracker/tracker.hpp src/tracker/worker.hpp ${UTIL_DEPS}
TRACKER_FLAGS = ${UTIL_FLAGS}
TRACKER_LIBS = ${UTIL_LIBS}


# compilations
run: build/tracker
	./build/tracker

build/tracker: src/tracker.cpp ${TRACKER_DEPS}
	${CPP} ${MD_FLAGS} -o build/tracker $< ${MD_LIBS}


# mix
.PHONY: clean
clean:
	rm -rf build/*
