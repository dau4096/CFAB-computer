CC = g++
INCLUDE = -I/usr/include -I/usr/local/include

LIBS = -lm -ldl -pthread

SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)
BINFILE = prgm.x86_64


.PHONY: all release debug clean

all: release


release: CFLAGS = -std=c++23 -O3 -march=native -ffast-math
release: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(BINFILE)


debug: CFLAGS = -std=c++23 -O0 -g3
debug: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $(BINFILE)



%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(BINFILE)

