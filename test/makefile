CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -g $(EXTRA_CXXFLAGS)

SRCS     := $(wildcard *.cpp)
OBJS     := $(SRCS:.cpp=.o)
TARGET   := test.bin

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LN_OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
