run: build/main
	./build/main

build/main: src/main.cpp
	g++ -o build/main src/main.cpp

.PHONY: clean
clean:
	rm -f build/main
