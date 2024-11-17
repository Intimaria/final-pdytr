#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <openssl/sha.h>

void calculate_checksum(const char *data, size_t len, unsigned char *output);

#endif // CHECKSUM_H

