
APPNAME := blackout
TARGET := $(APPNAME).elf

CC := ppu-gcc
CFLAGS := -O2 -Wall -Wextra
LDFLAGS :=

.PHONY: all clean

all: $(TARGET)

$(TARGET): src/main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
