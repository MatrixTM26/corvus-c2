CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -O2 -Icore

SRCS    = main.c \
          core/Config.c \
          core/Banner.c \
          core/Cipher.c \
          core/Session.c \
          core/Network.c \
          core/Console.c \
          core/transport/Tcp.c \
          core/transport/Http.c \
          core/transport/Tls.c

OBJS    = $(SRCS:.c=.o)
TARGET  = c2server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

ssl: CFLAGS += -DHAVE_OPENSSL
ssl: LDFLAGS += -lssl -lcrypto
ssl: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o:                    main.c core/Config.h core/Banner.h core/Session.h core/Network.h core/Console.h
core/Config.o:             core/Config.c core/Config.h
core/Banner.o:             core/Banner.c core/Banner.h core/Config.h
core/Cipher.o:             core/Cipher.c core/Cipher.h
core/Session.o:            core/Session.c core/Session.h
core/Network.o:            core/Network.c core/Network.h
core/Console.o:            core/Console.c core/Console.h core/Session.h core/Config.h
core/transport/Tcp.o:      core/transport/Tcp.c core/transport/Tcp.h
core/transport/Http.o:     core/transport/Http.c core/transport/Http.h
core/transport/Tls.o:      core/transport/Tls.c core/transport/Tls.h

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all ssl clean
