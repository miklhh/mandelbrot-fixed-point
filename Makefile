CC = g++
CFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Weffc++ -O3 -march=native

mandelbrot: main.cc render.h
	$(CC) $(CFLAGS) -o mandelbrot -lSDL main.cc
