CC = g++
CFLAGS = -std=c++23 -O3 -march=native -ffast-math

LIBS = -lm -ldl -pthread

SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: prgm

prgm: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o prgm

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) prgm

