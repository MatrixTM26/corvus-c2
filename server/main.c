#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#define StdinFd _fileno(stdin)
#else
#include <sys/select.h>
#define StdinFd STDIN_FILENO
#endif

#include "core/Banner.h"
#include "core/Config.h"
#include "core/Console.h"
#include "core/Network.h"
#include "core/Session.h"

int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    fprintf(stderr,
            "Usage: %s -s <host> -p <port> -m <tcp|http|https|tls|mtls> "
            "[options]\n",
            Argv[0]);
    fprintf(stderr,
            "  --cert <file>   TLS certificate  (default: certs/server.crt)\n");
    fprintf(stderr,
            "  --key  <file>   TLS private key  (default: certs/server.key)\n");
    fprintf(stderr,
            "  --ca   <file>   CA cert for mTLS (default: certs/ca.crt)\n");
    fprintf(stderr, "  --path <path>   HTTP beacon path (default: /update)\n");
    fprintf(stderr, "  --ua   <str>    User-Agent to match\n");
    fprintf(stderr, "  --beacon <ms>   Beacon interval  (default: 3000)\n");
    fprintf(stderr, "  --jitter <pct>  Jitter percent   (default: 15)\n");
    return 1;
  }

  Config C;
  ConfigDefaults(&C);

  if (!ConfigParse(Argc, Argv, &C))
    return 1;

  if (!NetInit()) {
    fprintf(stderr, "[!] Network init failed.\n");
    return 1;
  }

  NetHandle H;
  if (!NetStart(&C, &H)) {
    NetShutdown();
    return 1;
  }

  BannerPrint(&C);

  SessionPool Pool;
  PoolInit(&Pool);

  printf("server ~$ ");
  fflush(stdout);

  while (1) {
    fd_set Fds;
    FD_ZERO(&Fds);
    FD_SET(H.Listener, &Fds);
    FD_SET(StdinFd, &Fds);

#ifdef _WIN32
    int MaxFd = 0;
#else
    int MaxFd = (int)H.Listener > StdinFd ? (int)H.Listener : StdinFd;
#endif

    struct timeval Tv = {.tv_sec = 0, .tv_usec = 100000};

    if (select(MaxFd + 1, &Fds, NULL, NULL, &Tv) < 0)
      continue;

    if (FD_ISSET(H.Listener, &Fds))
      NetDispatch(&H, &Pool, &C);

    if (FD_ISSET(StdinFd, &Fds)) {
      char Line[BufSize] = {0};
      if (!ConsoleRead(Line, sizeof(Line)))
        continue;
      if (!ConsoleExec(Line, &Pool, &C))
        break;
    }
  }

  NetStop(&H);
  NetShutdown();
  return 0;
}
