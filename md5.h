#ifndef MD5_H_
#define MD5_H_
#include <stdio.h>
#include <openssl/md5.h>

#define CHUNK_SIZE 1024

typedef unsigned char MD5_HASH[MD5_DIGEST_LENGTH];

unsigned char *md5_get(FILE *file);

void md5_print(MD5_HASH c);

void md5_copy_hash(MD5_HASH dst, const MD5_HASH src);


#endif
