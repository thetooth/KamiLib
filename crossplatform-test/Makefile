C_FILES = 
CPP_FILES = demo01.cpp
OBJECTS = $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

CC = gcc
CPP = g++
CXXFLAGS = -Wall -std=c++11 -fPIC
INCLUDE = -I. -I/usr/include

EXEC = demo01

LDLIBS = -lX11 -lGL -lGLU
LDFLAGS = -L/usr/lib

%.o: %.c
	$(CC) -g -c $< -o $@

%.o: %.cpp
	$(CPP) -g -std=c++11 -c $< -o $@ -I../kami -L../kami -lkami

$(EXEC): $(OBJECTS)
	$(CPP) -g $(OBJECTS) -I../kami -I. -I/usr/include -L/usr/lib -lX11 -lGL -lGLU -L../kami -lkami -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)
