C_FILES = 
CPP_FILES = buildnumber.cpp
OBJECTS = $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

CC = clang
CPP = clang++
CXXFLAGS = -Wall -std=c++11 -fPIC
INCLUDE = -I. -I/usr/include

EXEC = buildnumber

LDLIBS = 
LDFLAGS = -L/usr/lib

%.o: %.c
	$(CC) -g -c $< -o $@

%.o: %.cpp
	$(CPP) -g -std=c++11 -c $< -o $@

$(EXEC): $(OBJECTS)
	$(CPP) -g $(OBJECTS) -I. -I/usr/include -L/usr/lib -o $(EXEC)

clean:
	rm -f $(EXEC) $(OBJECTS)
