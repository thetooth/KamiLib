C_FILES = 
CPP_FILES = demo01.cpp
OBJECTS = $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

CC = clang
CPP = clang++

INCLUDE = -I. -I/usr/include -I../kami
LDFLAGS = -L/usr/lib -L../kami -lX11 -lGL -lGLU -lkami
CFLAGS = -Wall -fPIC -O0 $(INCLUDE)
CXXFLAGS = -Wall -std=c++11 -fPIC -O0 $(INCLUDE)

EXEC = demo01

%.o: %.c
	$(CC) -g $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CPP) -g $(CXXFLAGS) -c $< -o $@

$(EXEC): $(OBJECTS)
	$(CPP) -g $(OBJECTS) $(CXXFLAGS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)
