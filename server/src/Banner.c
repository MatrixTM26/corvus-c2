#include "../include/Banner.h"
#include <stdio.h>

#define ColReset  "\033[0m"
#define ColRed    "\033[1;31m"
#define ColWhite  "\033[1;37m"
#define ColGray   "\033[0;90m"
#define ColYellow "\033[1;33m"
#define ColCyan   "\033[0;36m"
#define ColBlue   "\033[1;34m"

void BannerPrint(const Config *C)
{
    printf("\n");
    printf("%s", ColRed);
    printf("    ██████╗ ██████╗ ██████╗ ██╗   ██╗██╗   ██╗███████╗\n");
    printf("   ██╔════╝██╔═══██╗██╔══██╗██║   ██║██║   ██║██╔════╝\n");
    printf("   ██║     ██║   ██║██████╔╝██║   ██║██║   ██║███████╗\n");
    printf("   ██║     ██║   ██║██╔══██╗╚██╗ ██╔╝██║   ██║╚════██║\n");
    printf("   ╚██████╗╚██████╔╝██║  ██║ ╚████╔╝ ╚██████╔╝███████║\n");
    printf("    ╚═════╝ ╚═════╝ ╚═╝  ╚═╝  ╚═══╝   ╚═════╝ ╚══════╝\n");
    printf("%s\n", ColReset);

    printf("   %smode%s    %s%s%s    %sbind%s  %s%s:%d%s\n",
           ColGray,   ColReset,
           ColCyan,   ConfigModeName(C->Mode), ColReset,
           ColGray,   ColReset,
           ColYellow, C->BindAddr, C->Port, ColReset);

    printf("   %sbeacon%s  %s%dms%s    %sjitter%s  %s%d%%%s\n",
           ColGray, ColReset,
           ColCyan, C->BeaconMs, ColReset,
           ColGray, ColReset,
           ColCyan, C->JitterPct, ColReset);

    if (C->Mode >= ModeTls)
        printf("   %scert%s    %s%s%s\n",
               ColGray, ColReset, ColGray, C->CertFile, ColReset);

    if (C->Mode == ModeMtls)
        printf("   %sca%s      %s%s%s\n",
               ColGray, ColReset, ColGray, C->CaFile, ColReset);

    printf("\n   %stype 'help' for commands%s\n\n",
           ColGray, ColReset);
}
