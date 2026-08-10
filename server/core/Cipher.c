#include "Cipher.h"

void ApplyXor(char *Data, size_t Len) {
  for (size_t I = 0; I < Len; I++)
    Data[I] ^= XorKey;
}
