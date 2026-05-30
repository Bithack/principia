BIN = ../bin
PRINCIPIA = ../..
PROGRAM = lvledit
SRCS := main.cc $(PRINCIPIA)/src/pkgman.cc $(PRINCIPIA)/src/rand.c

CXX ?= g++
CXXFLAGS ?= -D_NO_TMS -O2 -ffunction-sections -fdata-sections
INCLUDES ?= -I$(PRINCIPIA)/src -I$(PRINCIPIA)/lib
LDFLAGS ?= -lz -Wl,--gc-sections

all: $(BIN)/$(PROGRAM)

$(BIN)/$(PROGRAM): $(SRCS)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	rm $(BIN)/$(PROGRAM)

.PHONY: all clean
