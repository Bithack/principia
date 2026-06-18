BIN = ../bin
PRINCIPIA = ../..
PROGRAM = package-creator
SRCS := main.cc $(PRINCIPIA)/src/pkgman.cc

# always static unless you do STATIC=0
STATIC ?= 1

CXX ?= g++
CXXFLAGS ?= -D_NO_TMS -O2 -ffunction-sections -fdata-sections
INCLUDES ?= -I$(PRINCIPIA)/src -I$(PRINCIPIA)/lib
LDFLAGS ?= -lz -Wl,--gc-sections
ifeq ($(STATIC),1)
	LDFLAGS += -l:libjansson.a
else
	LDFLAGS += -ljansson
endif

all: $(BIN)/$(PROGRAM)

$(BIN)/$(PROGRAM): $(SRCS)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	rm $(BIN)/$(PROGRAM)

.PHONY: all clean
