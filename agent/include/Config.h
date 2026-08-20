#ifndef AgentConfigH
#define AgentConfigH

#define BufSize    8192
#define MaxUaLen   256
#define MaxPathLen 512
#define AddrSize   128

typedef enum {
    ModeTcp   = 0,
    ModeHttp  = 1,
    ModeHttps = 2,
    ModeTls   = 3,
    ModeMtls  = 4
} TransportMode;

typedef struct {
    TransportMode Mode;
    char          Host[AddrSize];
    int           Port;
    char          HttpPath[MaxPathLen];
    char          UserAgent[MaxUaLen];
    char          CertFile[MaxPathLen];
    char          KeyFile[MaxPathLen];
    char          CaFile[MaxPathLen];
    int           BeaconMs;
    int           JitterPct;
} AgentConfig;

void AgentConfigDefaults(AgentConfig *C);
void AgentConfigParse(int Argc, char *Argv[], AgentConfig *C);
void AgentConfigUsage(const char *Bin);

#endif
