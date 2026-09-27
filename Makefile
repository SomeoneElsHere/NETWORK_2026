LFLAGS = -L src/libmd/

run: build/main
	./build/main

build/main: src/main.cpp src/util.h
	g++ ${LFLAGS} -o build/main src/main.cpp -lmd -static

.PHONY: clean
clean:
	rm -rf build/*
