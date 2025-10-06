# Makefile for yash

CC = gcc
CFLAGS = -Wall -Wextra -std=c99  # Add -g for debugging if needed
TARGET = yash
SOURCES = main.c signal.c tokenizer.c utils.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = shell.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
