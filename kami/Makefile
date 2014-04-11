C_FILES = gl_core_3_3.c xxhash.c
CPP_FILES = lodepng.cpp unix.cpp kami.cpp
OBJECTS = $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

CC = clang
CPP = clang++

INCLUDE = -I. -I/usr/include
CFLAGS = -Wall -fPIC -O0 $(INCLUDE)
CXXFLAGS =  -std=c++11 -fPIC -O0 $(INCLUDE)

EXEC = kami

%.o: %.c
	$(CC) -g $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CPP) -g $(CXXFLAGS) -c $< -o $@

$(EXEC): $(OBJECTS)
	ar rvcs lib$(EXEC).a $(OBJECTS)

clean:
	rm -f $(EXEC) libkami.a $(OBJECTS)
