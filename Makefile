CC      = clang
CFLAGS  = -Wall -Wextra -Wpedantic -Wextra
LFLAGS  = -lncurses
TARGETS = minesweeper 
MODULES = 
OBJECTS = $(patsubst %,%.o,$(MODULES))
TOBJECTS= $(patsubst %,%.o,$(TARGETS))      # target objects

.PHONY: all debug clean tidy

all: $(TARGETS)

debug: CFLAGS += -g
debug: $(TARGETS)

$(TARGETS): %: %.o $(OBJECTS)
	$(CC) -o $@ $^ $(LFLAGS) $(CFLAGS)

%.o: %.c %.h
	$(CC) -c $< $(CFLAGS)

$(TOBJECTS): %.o: %.c
	$(CC) -c $< $(CFLAGS)

clean: tidy
	$(RM) $(TARGETS)

tidy:
	$(RM) $(OBJECTS) $(patsubst %,%.o,$(TARGETS))
