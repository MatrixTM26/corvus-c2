#include "../include/Identity.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
    #pragma comment(lib, "advapi32.lib")
#else
    #include <unistd.h>
    #include <fcntl.h>
#endif

static void FillRandom(unsigned char *Buf, int Len)
{
#ifdef _WIN32
    HCRYPTPROV Prov;
    if (CryptAcquireContext(&Prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(Prov, Len, Buf);
        CryptReleaseContext(Prov, 0);
        return;
    }
#else
    int Fd = open("/dev/urandom", O_RDONLY);
    if (Fd >= 0) {
        int R = (int)read(Fd, Buf, (size_t)Len); (void)R;
        close(Fd);
        return;
    }
#endif
    for (int I = 0; I < Len; I++)
        Buf[I] = (unsigned char)(rand() % 256);
}

void IdentityGenerate(char *Out)
{
    unsigned char B[16];
    FillRandom(B, 16);

    B[6] = (B[6] & 0x0F) | 0x40;
    B[8] = (B[8] & 0x3F) | 0x80;

    snprintf(Out, UuidLen,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             B[0],B[1],B[2],B[3], B[4],B[5], B[6],B[7],
             B[8],B[9], B[10],B[11],B[12],B[13],B[14],B[15]);
}
