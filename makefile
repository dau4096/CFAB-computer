CC = g++
CFLAGS = -std=c++23 -O2 -ffast-math

LIBS = -lm -ldl -pthread

SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: app

app: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o app

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) app

