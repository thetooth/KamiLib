C_FILES = ini.c json.c
CPP_FILES = inireader.cpp picopng.cpp tween.cpp unix.cpp kami.cpp
OBJECTS = $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

CC = gcc
CPP = g++
CXXFLAGS = -Wall -std=c++11 -fPIC
INCLUDE = -I. -I/usr/include

EXEC = kami

LDLIBS = -lX11 -lGL -lGLU
LDFLAGS = -L/usr/lib

%.o: %.c
	$(CC) -g -c $< -o $@

%.o: %.cpp
	$(CPP) -g -std=c++11 -c $< -o $@

$(EXEC): $(OBJECTS)
	ar rvcs lib$(EXEC).a $(OBJECTS)

clean:
	rm -f $(EXEC) libkami.a
