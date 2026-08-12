# Simple C2

simple command and control (c2) server written in C

## Build

```bash
cd server && make        # -> ./c2server
cd agent  && make        # -> ./agent
```

## Run

```bash
./server/c2server -s 0.0.0.0 -p 4444 -m http
./agent/agent -s 127.0.0.1 -p 4444 -m http
```

## Console Commands

```txt
help              Show help
sessions          List active agents          (alias: session)
use 1             Enter interactive shell     (alias: interact 1)
back              Return to main console
kill              Kill connected agent
clear             Clear screen
exit / quit       Shutdown server
```

## Contributors

<div align="center">
    <a href="https://github.com">
        <img src="https://contrib.rocks" />
    </a>
</div>


<p align="center">&copy; 2023-2026 MatrixTM26</p>
