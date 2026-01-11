CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Os -I./include
LDFLAGS =
TARGET = joshbox
SOURCES = src/ls.c src/cp.c src/mv.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

strip: $(TARGET)
	strip $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

.PHONY: all clean strip install
