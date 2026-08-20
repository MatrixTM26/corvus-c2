#ifndef ConfigH
#define ConfigH

#define BufSize      8192
#define AddrSize     128
#define MaxSessions  64
#define MaxUaLen     256
#define MaxPathLen   512

typedef enum {
    ModeTcp   = 0,
    ModeHttp  = 1,
    ModeHttps = 2,
    ModeTls   = 3,
    ModeMtls  = 4
} TransportMode;

typedef struct {
    TransportMode Mode;
    char          BindAddr[AddrSize];
    int           Port;
    char          UserAgent[MaxUaLen];
    char          HttpPath[MaxPathLen];
    char          CertFile[MaxPathLen];
    char          KeyFile[MaxPathLen];
    char          CaFile[MaxPathLen];
    int           BeaconMs;
    int           JitterPct;
} Config;

void        ConfigDefaults(Config *C);
const char *ConfigModeName(TransportMode M);
int         ConfigParse(int Argc, char *Argv[], Config *C);
void        ConfigPrint(const Config *C);

#endif
