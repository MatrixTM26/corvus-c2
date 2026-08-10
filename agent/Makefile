CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2
TARGET = agent

all: $(TARGET)

$(TARGET): agent.c
	$(CC) $(CFLAGS) -o $@ $^

ssl: agent.c
	$(CC) $(CFLAGS) -DHAVE_OPENSSL -o $(TARGET) $^ -lssl -lcrypto

clean:
	rm -f $(TARGET)

.PHONY: all ssl clean
