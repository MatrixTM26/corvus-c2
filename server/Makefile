CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2 -Icore

SRCS   = main.c \
         core/Cipher.c \
         core/Session.c \
         core/Network.c \
         core/Console.c

OBJS   = $(SRCS:.c=.o)
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o:          main.c           core/Cipher.h core/Session.h core/Network.h core/Console.h
core/Cipher.o:   core/Cipher.c    core/Cipher.h
core/Session.o:  core/Session.c   core/Session.h
core/Network.o:  core/Network.c   core/Network.h core/Cipher.h core/Session.h
core/Console.o:  core/Console.c   core/Console.h core/Session.h

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild
