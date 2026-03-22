# COS344 Practical 2
# Removed glad.c - project uses GLEW (-lGLEW) which is already installed.
# glad.c is only needed if you #include <glad/glad.h>, which this project doesn't.

files = shader.cpp Triangle.cpp Square.cpp

CXX = g++
CXXFLAGS = -std=c++11 -g -Wall

LIBS = -lglfw -lGLEW -lGL -ldl -lpthread

main: main.cpp $(files)
	$(CXX) $(CXXFLAGS) main.cpp $(files) $(LIBS) -o main

run: main
	./main

clean:
	rm -f *.o main

all:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) run