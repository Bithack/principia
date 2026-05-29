BIN = ../bin
PRINCIPIA = ../..
PROGRAM = progress-get
SRCS := main.cc $(PRINCIPIA)/src/progress.cc

CXX ?= g++
CXXFLAGS ?= -D_NO_TMS -O2
INCLUDES ?= -I$(PRINCIPIA)/src -I$(PRINCIPIA)/lib
LDFLAGS ?= -lz

all: $(BIN)/$(PROGRAM)

$(BIN)/$(PROGRAM): $(SRCS)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	rm $(BIN)/$(PROGRAM)

.PHONY: all clean
