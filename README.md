# Simple C2 v3

## Structure

```
server/
├── main.c
├── Makefile
└── core/
    ├── Cipher.c/.h
    ├── Session.c/.h
    ├── Network.c/.h
    └── Console.c/.h

agent/
├── agent.c
└── Makefile
```

## Build

```bash
cd server && make        # -> ./c2server
cd agent  && make        # -> ./agent
```

## Run

```bash
./server/c2server -s 0.0.0.0 -p 4444
./agent/agent -s <ip> -p 4444
```

## Console Commands

```
help              Show help
sessions          List active agents          (alias: session)
use 1             Enter interactive shell     (alias: interact 1)
back              Return to main console
kill              Kill connected agent
clear             Clear screen
exit / quit       Shutdown server
```
