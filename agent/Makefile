CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2
TARGET = agent

all: $(TARGET)

$(TARGET): agent.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
