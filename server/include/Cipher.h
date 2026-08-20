#ifndef CipherH
#define CipherH

#include <stddef.h>

#define XorKey 0x5A

void ApplyXor(char *Data, size_t Len);

#endif
