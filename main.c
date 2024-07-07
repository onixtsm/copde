#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CHUNK_SIZE 1024

typedef struct {
  char path[256];
  unsigned char key[MD5_DIGEST_LENGTH];
  size_t value;
} Item;

typedef struct {
  Item **items;
  size_t size;
  size_t capacity;
} Set;

Set *set_init(void) {
  Set *set = malloc(sizeof(*set) * CHUNK_SIZE);
  set->capacity = CHUNK_SIZE;
  set->size = 0;
  set->items = malloc(sizeof(Item) * CHUNK_SIZE);
  return set;
}

void set_expand(Set *set) {
  printf("size: %zu, cap: %zu\n", set->size, set->capacity);
  size_t t = set->size;
  set = reallocarray(set, sizeof(Item), set->capacity + CHUNK_SIZE);
  set->capacity = set->capacity + CHUNK_SIZE ;
  set->size = t;
}

void set_add(Set *set, unsigned char *key, const char *path) {
  for (size_t i = 0; i < set->size; ++i) {
    if (strcmp((char *)set->items[i]->key, (char *)key) == 0) {
      set->items[i]->value++;
      return;
    }
  }
  if (set->capacity == set->size) {
    set_expand(set);
  }
  size_t index = set->size;
  printf("%zu:%s\n", index, path);
  set->items[index] = malloc(sizeof(Item));
  if (set->items[index] == NULL) {
    printf("[ERROR] Buy more ram\n");
    return;
  }
  strcpy((char *)set->items[index]->key, (char *)key);
  strcpy(set->items[index]->path, path);
  set->items[index]->value = 0;
  set->size++;
}

void set_print_duplicates(Set *set) {
  for (size_t i = 0; i < set->size; ++i) {
    if (set->items[i]->value > 0) {
      printf("%s\n", set->items[0]->path);
    }
  }
}

void set_delete(Set *set) {
  for (size_t i = 0; i < set->size; ++i) {
    free(set->items[i]);
  }
  free(set);
}

unsigned char *get_md5(FILE *file) {
  unsigned char *c = malloc(sizeof(*c) * MD5_DIGEST_LENGTH);
  if (c == NULL) {
    printf("Buy more RAM\n");
    return NULL;
  }
  MD5_CTX ctx;
  size_t bytes;
  unsigned char data[CHUNK_SIZE];
  MD5_Init(&ctx);
  while ((bytes = fread(data, 1, CHUNK_SIZE, file)) != 0) {
    MD5_Update(&ctx, data, bytes);
  }
  MD5_Final(c, &ctx);
  return c;
}

void print_md5(unsigned char *c) {
  for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    printf("%02x", c[i]);
  }
}

bool is_dir(const char *path) {
  int fs = open(path, O_RDONLY);
  struct stat s;
  fstat(fs, &s);
  return S_ISDIR(s.st_mode);
}

void add_file(Set *set, const char *path) {
  FILE *f = fopen(path, "rb");
  unsigned char *c = get_md5(f);
  set_add(set, c, path);
  free(c);
  fclose(f);
}

void traverse_dir(Set *set, const char *path) {
  struct dirent *de;
  DIR *dir = opendir(path);
  if (dir == NULL) {
    printf("Could not open dir %s, because of %s\n", path, strerror(errno));
    return;
  }
  while ((de = readdir(dir)) != NULL) {
    if (de->d_type == DT_DIR && de->d_name[0] != '.') {
      char full_path[256] = {0};
      strncpy(full_path, path, strlen(path));
      full_path[strlen(path)] = '/';
      strncat(full_path, de->d_name, 255); 
      traverse_dir(set, full_path);
    } else if (de->d_type == DT_REG) {
      char full_path[256] = {0};
      strncpy(full_path, path, strlen(path));
      full_path[strlen(path)] = '/';
      strncat(full_path, de->d_name, 255); 
      add_file(set, full_path);
    }
  }
  closedir(dir);
}

int main(int argc, char **argv) {
  Set *set = set_init();
  while (argc > 1) {
    const char *path = argv[--argc];
    if (is_dir(path)) {
      traverse_dir(set, path);
      continue;
    }
    add_file(set, path);
  }
  set_print_duplicates(set);
  set_delete(set);
  return 0;
}
