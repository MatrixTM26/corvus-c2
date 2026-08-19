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

TARGET  = c2server

# --------------------------------------------------------------------------
# Plain build (tcp + http only — no TLS)
# --------------------------------------------------------------------------
all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) -o $@ $(SRCS)

# --------------------------------------------------------------------------
# TLS build  (requires libssl-dev / openssl-devel)
# Compiles every source file with -DHAVE_OPENSSL so the #ifdef guards are
# active in ALL translation units, then links against OpenSSL.
# --------------------------------------------------------------------------
ssl:
	$(CC) $(CFLAGS) -DHAVE_OPENSSL -o $(TARGET) $(SRCS) -lssl -lcrypto

clean:
	rm -f $(TARGET) core/transport/*.o core/*.o *.o

.PHONY: all ssl clean
