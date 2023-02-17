#include <dirent.h>
#include <errno.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>

#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))
#define clear_line() printf("\33[2K\r")
#define goup() printf("\033[A")
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
    hs->hashes =
        realloc(hs->hashes, (hs->cap * MD5_LEN * sizeof(char)) +
                                (REALLOC_SIZE * MD5_LEN * sizeof(char)));

    hs->cap += REALLOC_SIZE;
  }

  for (size_t i = 0; i < MD5_LEN; ++i) {
    // printf("%d\n", hash[i]);
    hs->hashes[hs->size * MD5_LEN + i] = hash[i];
  }
  hs->size++;

  return 0;
}

void readDir(char *dir_path, FILE *output_file) {

  struct dirent *pDrent;

  DIR *dir = opendir(dir_path);

  if (dir == NULL) {
    perror("Cannot open directory");
    closedir(dir);
    return;
  }

  // size_t num = 1;


  HashSet hashSet = hashSetInit();
  char full_path[100] = {'\0'};

  while ((pDrent = readdir(dir)) != NULL) {
    // for (size_t i = 0; i < 100; i++) {
    //   if (full_path[i] == '\0')
    //     break;
    //   full_path[i] = '\0';
    // }
    memset(&full_path, '\0', 100);
    strcat(full_path, dir_path);
    strcat(full_path, "/");
    strcat(full_path, pDrent->d_name);
    strcat(full_path, "\0");

    if (strcmp(pDrent->d_name, ".") == 0)
      continue;
    if (strcmp(pDrent->d_name, "..") == 0)
      continue;

    // printf("Working on file nr. %zu: %s \n", num, full_path);
    //
    FILE *file = fopen(full_path, "rb");
    if (file == NULL) {
      fprintf(stderr, "Error : Failed to open %s - %s\n", full_path,
              strerror(errno));
    }
    if (hashSetAdd(&hashSet, file) == 1) {
      duplicate(full_path, output_file);

    // } else {
      // goup();
      // clear_line();
    }
    fclose(file);
    // num++;
  }
  closedir(dir);
  hashSetDestroY(&hashSet);
}

int main(int argv, char **argc) {

  if (argv < 2) {
    printf("Usage: copde <dir> [file]\n");
    return 1;
  }

  char *dir = argc[1];

  if (argv > 2) {
    FILE *output_file = fopen(argc[2], "rb");
    if (output_file == NULL) {
      fprintf(stderr, "Could not open file %s - %s", argc[2], strerror(errno));
      return 1;
    }
    readDir(dir, output_file);
    fclose(output_file);
  } else {
    readDir(dir, stdout);
  }

  return 0;
}
