#include "checksum.h"

void calculate_checksum(const char *data, size_t len, unsigned char *output) {
    SHA512_CTX sha512;
    SHA512_Init(&sha512);
    SHA512_Update(&sha512, data, len);
    SHA512_Final(output, &sha512);
}
