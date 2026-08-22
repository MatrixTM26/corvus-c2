CC      = gcc
STD     = -std=c11
WARN    = -Wall -Wextra -pedantic
OPT     = -O2

SRVDIR  = server
AGNTDIR = agent
BLDDIR  = build

SRVINC  = -I$(SRVDIR)/include
AGNTINC = -I$(AGNTDIR)/include

SRVSRCS = $(SRVDIR)/src/Main.c     \
          $(SRVDIR)/src/Config.c   \
          $(SRVDIR)/src/Banner.c   \
          $(SRVDIR)/src/Cipher.c   \
          $(SRVDIR)/src/Log.c      \
          $(SRVDIR)/src/Session.c  \
          $(SRVDIR)/src/Console.c  \
          $(SRVDIR)/src/Network.c  \
          $(SRVDIR)/src/Tcp.c      \
          $(SRVDIR)/src/Http.c     \
          $(SRVDIR)/src/Tls.c

AGNTSRCS = $(AGNTDIR)/src/Main.c    \
           $(AGNTDIR)/src/Config.c   \
           $(AGNTDIR)/src/Identity.c \
           $(AGNTDIR)/src/Beacon.c   \
           $(AGNTDIR)/src/Exec.c

SRVOBJDIR  = $(BLDDIR)/server
AGNTOBJDIR = $(BLDDIR)/agent

SRVOBJS  = $(patsubst $(SRVDIR)/src/%.c,  $(SRVOBJDIR)/%.o,  $(SRVSRCS))
AGNTOBJS = $(patsubst $(AGNTDIR)/src/%.c, $(AGNTOBJDIR)/%.o, $(AGNTSRCS))

SRVBIN   = $(SRVOBJDIR)/c2server
AGNTBIN  = $(AGNTOBJDIR)/agent

AGENTS  ?= 1

.PHONY: all server agent ssl ssl-server ssl-agent certs clean help

all: server agent

server: $(SRVBIN)

agent: $(AGNTBIN)

$(SRVOBJDIR)/%.o: $(SRVDIR)/src/%.c
	@mkdir -p $(SRVOBJDIR)
	$(CC) $(STD) $(WARN) $(OPT) $(SRVINC) -c $< -o $@

$(AGNTOBJDIR)/%.o: $(AGNTDIR)/src/%.c
	@mkdir -p $(AGNTOBJDIR)
	$(CC) $(STD) $(WARN) $(OPT) $(AGNTINC) -c $< -o $@

$(SRVBIN): $(SRVOBJS)
	@mkdir -p $(SRVOBJDIR)
	$(CC) $(STD) $(WARN) $(OPT) -o $@ $^

$(AGNTBIN): $(AGNTOBJS)
	@mkdir -p $(AGNTOBJDIR)
	$(CC) $(STD) $(WARN) $(OPT) -o $@ $^

ssl: ssl-server ssl-agent

ssl-server:
	@mkdir -p $(SRVOBJDIR)
	$(CC) $(STD) $(WARN) $(OPT) $(SRVINC) -DHAVE_OPENSSL \
	    -o $(SRVBIN) $(SRVSRCS) -lssl -lcrypto

ssl-agent:
	@mkdir -p $(AGNTOBJDIR)
	$(CC) $(STD) $(WARN) $(OPT) $(AGNTINC) -DHAVE_OPENSSL \
	    -o $(AGNTBIN) $(AGNTSRCS) -lssl -lcrypto

certs:
	@bash tools/GenCert.sh --agents $(AGENTS)

clean:
	rm -rf $(BLDDIR)

help:
	@echo ""
	@echo "  Targets"
	@echo "  -------"
	@echo "  all          Build server + agent (plain tcp/http)"
	@echo "  server       Build server only"
	@echo "  agent        Build agent only"
	@echo "  ssl          Build server + agent with TLS/mTLS support"
	@echo "  ssl-server   Build server only with TLS/mTLS"
	@echo "  ssl-agent    Build agent only with TLS/mTLS"
	@echo "  certs        Generate certificates (use AGENTS=n for multi-agent)"
	@echo "  clean        Remove build directory"
	@echo ""
	@echo "  Examples"
	@echo "  --------"
	@echo "  make ssl                        Build with TLS support"
	@echo "  make certs AGENTS=3             Generate CA + server + 3 agent certs"
	@echo "  make ssl AGENTS=3 certs         Build TLS + generate 3 agent certs"
	@echo ""
	@echo "  Run (mTLS)"
	@echo "  ----------"
	@echo "  ./build/server/c2server -s 0.0.0.0 -p 4444 -m mtls \\"
	@echo "    --cert certs/server/server.crt \\"
	@echo "    --key  certs/server/server.key \\"
	@echo "    --ca   certs/server/ca.crt"
	@echo ""
	@echo "  ./build/agent/agent -s <IP> -p 4444 -m mtls \\"
	@echo "    --cert certs/agent-1/agent.crt \\"
	@echo "    --key  certs/agent-1/agent.key \\"
	@echo "    --ca   certs/agent-1/ca.crt"
	@echo ""
