build:
	g++ -g -o src/main.exe src/*.cpp

run: build
	./src/main.exe