#include "./md5.h"

#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char *md5_get(FILE *file) {
  unsigned char *c = malloc(sizeof(*c) * MD5_DIGEST_LENGTH);
  if (c == NULL) {
    fprintf(stderr, "[ERROR] Buy more RAM\n");
    return NULL;
  }
  MD5_CTX ctx;
  size_t bytes;
  char data[CHUNK_SIZE];
  MD5_Init(&ctx);
  while ((bytes = fread(data, 1, CHUNK_SIZE, file)) != 0) {
    MD5_Update(&ctx, data, bytes);
  }
  MD5_Final(c, &ctx);
  return c;
}

void md5_print(MD5_HASH c) {
  for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    printf("%02x", c[i]);
  }
  fflush(stdout);
}

void md5_copy_hash(MD5_HASH dst, const MD5_HASH src) { strcpy((char *)dst, (const char *)src); }
