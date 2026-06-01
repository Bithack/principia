BIN = ../bin
PRINCIPIA = ../..
PROGRAM = featured-list-creator
SRCS := main.c

# always static unless you do STATIC=0
STATIC ?= 1

CC ?= gcc
CFLAGS ?= -O2 -ffunction-sections -fdata-sections
LDFLAGS ?= -Wl,--gc-sections
ifeq ($(STATIC),1)
	LDFLAGS += -l:libjansson.a
else
	LDFLAGS += -ljansson
endif

all: $(BIN)/$(PROGRAM)

$(BIN)/$(PROGRAM): $(SRCS)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	rm $(BIN)/$(PROGRAM)

.PHONY: all clean
