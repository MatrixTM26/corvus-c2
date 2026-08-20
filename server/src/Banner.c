#include "../include/Banner.h"
#include <stdio.h>

void BannerPrint(const Config *C)
{
    printf("\033[1;31m");
    printf("                                                    \n");
    printf("   ██████╗██████╗      ███████╗██████╗ █████╗ \n");
    printf("  ██╔════╝╚════██╗     ██╔════╝██╔══██╗██╔══██╗\n");
    printf("  ██║      █████╔╝     █████╗  ██████╔╝███████║\n");
    printf("  ██║     ██╔═══╝      ██╔══╝  ██╔══██╗██╔══██║\n");
    printf("  ╚██████╗███████╗     ██║     ██║  ██║██║  ██║\n");
    printf("   ╚═════╝╚══════╝     ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝\n");
    printf("\033[0m");
    printf("\033[90m  C2 Framework — Educational / Red Team Lab Use Only\033[0m\n");
    printf("\n");
    printf("\033[1;37m  Listener\033[0m\n");
    ConfigPrint(C);
    printf("\n");
    printf("\033[90m  Type 'help' for commands.\033[0m\n");
    printf("\n");
}
