CC = gcc
STRIP = strip
FLAGS = -Wall
SOURCE_FILES = rrep.c
BIN_NAME = rrep

default: $(BIN_NAME)

$(BIN_NAME):
	$(CC) $(FLAGS) -o $@ $(SOURCE_FILES)
	$(STRIP) $@

clean:
	rm -f $(BIN_NAME)
