# Simple C2 — Refactored

## Structure

```
server/
├── main.c              Entry point + select() event loop
├── Makefile
└── core/
    ├── Cipher.c/.h     XOR obfuscation (key 0x5A)
    ├── Session.c/.h    Agent session state machine
    ├── Network.c/.h    Listener, beacon handler
    └── Console.c/.h    Operator terminal / CLI

beacon/
└── beacon.c            Agent (unchanged)
```

## Build

```bash
cd server && make
gcc -o beacon beacon/beacon.c
```

## Usage

```bash
./server/server -s 0.0.0.0 -p 8080
./beacon -s 127.0.0.1 -p 8080
```

## Console Commands

| Command        | Action                        |
|---------------|-------------------------------|
| `help`         | Show command reference        |
| `session`      | List registered agents        |
| `use 1`        | Enter interactive shell       |
| `back`         | Return to main console        |
| `clear`        | Clear terminal                |
| `exit`         | Kill agent + shutdown server  |
