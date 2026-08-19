CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
TARGET  = agent

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) -o $@ agent.c

ssl:
	$(CC) $(CFLAGS) -DHAVE_OPENSSL -o $(TARGET) agent.c -lssl -lcrypto

clean:
	rm -f $(TARGET)

.PHONY: all ssl clean
