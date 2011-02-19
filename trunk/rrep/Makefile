CC = gcc
STRIP = strip
FLAGS = -Wall
OBJECTS = messages.o bufferio.o rrep.o
BIN_NAME = rrep

default: $(BIN_NAME)

$(BIN_NAME): $(OBJECTS)
	$(CC) $(FLAGS) -o $@ $(OBJECTS)
	$(STRIP) $@

%.o: %.cpp
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm -f $(BIN_NAME) $(OBJECTS)
