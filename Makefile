CXXFLAGS=-g -O2 -DUSE_WAYLAND -Wall -std=c++20 -I . -I include $(shell pkg-config --cflags fontconfig)
LDFLAGS=-L . $(shell pkg-config --libs fontconfig)
OBJ=$(shell find -name "*.cc" | sed -e 's/\.cc$$/.o/g') $(shell find src -name "*.cpp" | sed -e 's/\.cpp$$/.o/g')
TARGET=test.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJ)
