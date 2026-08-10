#ifndef CIPHER_H
#define CIPHER_H

#include <stddef.h>

#define XorKey 0x5A

void ApplyXor(char *Data, size_t Len);

#endif
