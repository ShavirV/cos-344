# COS344 Practical 3
# ==================
# Ensure shader.cpp and shader.hpp are in the same directory.
# glad.c is optional (included if present).

CXX      = g++
CXXFLAGS = -std=c++11 -g -Wall -Iinclude

GLAD = $(wildcard glad.c)
LIBS = -lglfw -lGLEW -lGL -ldl -lpthread

main: main.cpp shader.cpp $(GLAD)
	$(CXX) $(CXXFLAGS) main.cpp $(GLAD) shader.cpp $(LIBS) -o main

run: main
	./main

clean:
	rm -f main *.o

all:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) run