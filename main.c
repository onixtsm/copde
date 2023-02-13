#include <dirent.h>
#include <errno.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MD5_LEN 32

typedef struct HashSet {
  char *hashes;
  size_t size;
  size_t cap;
} HashSet;

char *getMD5_file(FILE *item) {

  unsigned char c[MD5_DIGEST_LENGTH];
  int i;
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];
  char *filemd5 = (char *)malloc(33 * sizeof(char));

  MD5_Init(&mdContext);

  while ((bytes = fread(data, 1, 1024, item)) != 0)

    MD5_Update(&mdContext, data, bytes);

  MD5_Final(c, &mdContext);

  for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&filemd5[i * 2], "%02x", (unsigned int)c[i]);
  }

  return filemd5;
}

#define INITIAL_SIZE 250

void duplicate(char *path, FILE *sink) { fprintf(sink, "%s\n", path); }

HashSet hashSetInit(void) {
  HashSet hash_set;
  hash_set.hashes = malloc(sizeof(char) * MD5_LEN * INITIAL_SIZE);
  hash_set.size = 0;
  hash_set.cap = INITIAL_SIZE;
  return hash_set;
}

void hashSetDestroY(HashSet *hs) {
  if (hs->hashes) {
    free(hs->hashes);
  }
}

int hashSetAdd(HashSet *hs, void *item) {
  if (hs == NULL) {
    return 1;
  }

  const char *hash = getMD5_file((FILE *)item);
  // printf("%s\n", hash);
  for (size_t i = 0; i < hs->size; i++) {
    for (size_t j = 0; j < MD5_LEN; j++) {
      if (hash[j] != hs->hashes[MD5_LEN * i + j])
        break;
      else if (j == 15) {
        return 1;
      }
    }
  }
  #define REALLOC_SIZE 300

  if (hs->cap - 1 == hs->size) {
     hs->hashes = realloc(hs->hashes, (hs->cap * MD5_LEN * sizeof(char)) + (REALLOC_SIZE * MD5_LEN * sizeof(char)));
    // void *tmp = malloc(hs->cap + (REALLOC_SIZE * MD5_LEN));
    // hs->hashes = memcpy(tmp, hs->hashes, hs->size * MD5_LEN);

    hs->cap += REALLOC_SIZE;
  }

  for (size_t i = 0; i < MD5_LEN; ++i) {
    // printf("%d\n", hash[i]);
    hs->hashes[hs->size * MD5_LEN + i] = hash[i];
  }
  hs->size++;

  return 0;
}

void readDir(char *dir_path) {

  struct dirent *pDrent;

  DIR *dir = opendir(dir_path);

  if (dir == NULL) {
    perror("Cannot open directory");
  }

  HashSet hashSet = hashSetInit();
  char full_path[100] = {'\0'};

  while ((pDrent = readdir(dir)) != NULL) {
    for (size_t i = 0; i < 100; i++) {
      if (full_path[i] == '\0')
        break;
      full_path[i] = '\0';
    }
    strcat(full_path, dir_path);
    strcat(full_path, "/");
    strcat(full_path, pDrent->d_name);
    strcat(full_path, "\0");
    // printf("%s: ", full_path);

    if (strcmp(pDrent->d_name, ".") == 0)
      continue;
    if (strcmp(pDrent->d_name, "..") == 0)
      continue;

    FILE *file = fopen(full_path, "rb");
    if (file == NULL) {
      fprintf(stderr, "Error : Failed to open %s - %s\n", full_path,
              strerror(errno));
    }
    if (hashSetAdd(&hashSet, file) == 1) {
      duplicate(full_path, stdout);
    }
    fclose(file);
  }
  closedir(dir);
  hashSetDestroY(&hashSet);
}

int main(int argv, char **argc) {

  if (argv != 2) {
    printf("Usage: copde <dir>\n");
    return 1;
  }

  char *dir = argc[1];

  readDir(dir);

  return 0;
}
