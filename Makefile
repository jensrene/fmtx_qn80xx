CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g
LIBS = -lwiringPi

TARGET = fmtx
SRCS = main.cpp transmitter.cpp i2chandler.cpp rdsencoder.cpp conffile.cpp pwmcontroller.cpp strarray.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

