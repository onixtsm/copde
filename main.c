#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHUNK_SIZE 1024
#define PATHS_MAX 20
#define PATH_SIZE PATH_MAX

bool all_read = false;
atomic_char dp[PATH_SIZE];

const char SPIN[] = "┤ ┘ ┴ └ ├ ┌ ┬ ┐";
const size_t SPIN_SIZE = sizeof(SPIN) / sizeof(SPIN[0]);
const char MOVE_TO_BEGINING[] = "\r";

typedef struct {
  char main_path[PATH_SIZE];
  char paths[PATHS_MAX][PATH_SIZE];
  unsigned char key[MD5_DIGEST_LENGTH];
  size_t value;
} Item;

typedef struct {
  Item **items;
  atomic_size_t size;
  size_t capacity;
} Set;

struct Arg {
  int argc;
  char **argv;
  Set *set;
};

Set *set_init(void) {
  Set *set = malloc(sizeof(*set) * CHUNK_SIZE);
  set->capacity = CHUNK_SIZE;
  set->size = 0;
  set->items = malloc(sizeof(Item) * CHUNK_SIZE);
  if (set->items == NULL) {
    fprintf(stderr, "[ERROR] Buy more ram\n");
    exit(1);
  }
  return set;
}

void set_expand(Set *set) {
  size_t t = set->size;
  set->items = reallocarray(set->items, set->capacity + CHUNK_SIZE, sizeof(Item));
  set->capacity = set->capacity + CHUNK_SIZE;
  set->size = t;
}

void set_add(Set *set, unsigned char *key, const char *path) {
  // printf("%s\n", path);
  for (size_t i = 0; i < set->size; ++i) {
    if (strcmp((char *)set->items[i]->key, (char *)key) == 0) {
      strcpy(set->items[i]->paths[set->items[i]->value], path);
      set->items[i]->value++;
      return;
    }
  }
  if (set->capacity == set->size) {
    set_expand(set);
  }
  size_t index = set->size;
  set->items[index] = malloc(sizeof(Item));
  if (set->items[index] == NULL) {
    fprintf(stderr, "[ERROR] Buy more ram\n");
    return;
  }
  strcpy((char *)set->items[index]->key, (char *)key);
  strcpy(set->items[index]->main_path, path);
  set->items[index]->value = 0;
  set->size++;
}

void set_print_duplicates_main_paths(Set *set) {
  for (size_t i = 0; i < set->size; ++i) {
    if (set->items[i]->value > 0) {
      printf("'%s'\n", set->items[i]->main_path);
    }
  }
}

void set_print_all_files(Set *set, FILE *sink) {
  for (size_t i = 0; i < set->size; ++i) {
    Item *item = set->items[i];
    for (size_t k = 0; k < MD5_DIGEST_LENGTH; ++k) {
      fprintf(sink, "%02x", item->key[k]);
    }
    fprintf(sink, " '%s'", item->main_path);
    for (size_t j = 0; j < set->items[i]->value; ++j) {
      fprintf(sink, " '%s'", item->paths[j]);
    }
    fprintf(sink, "\n");
  }
}

void set_print_all_duplicates(Set *set, FILE *sink) {
  for (size_t i = 0; i < set->size; ++i) {
    if (set->items[i]->value != 0) {
      Item *item = set->items[i];
      for (size_t k = 0; k < MD5_DIGEST_LENGTH; ++k) {
        fprintf(sink, "%02x", item->key[k]);
      }
      fprintf(sink, " '%s'", item->main_path);
      for (size_t j = 0; j < set->items[i]->value; ++j) {
        fprintf(sink, " '%s'", item->paths[j]);
      }
      fprintf(sink, "\n");
    }
  }
}

void set_print_all_duplicates_paths(Set *set, FILE *sink) {
  for (size_t i = 0; i < set->size; ++i) {
    if (set->items[i]->value != 0) {
      Item *item = set->items[i];
      for (size_t j = 0; j < set->items[i]->value; ++j) {
        fprintf(sink, "'%s' ", item->paths[j]);
      }
      fprintf(sink, "\n");
    }
  }
}

void set_delete(Set *set) {
  for (size_t i = 0; i < set->capacity / CHUNK_SIZE; ++i) {
    free(set->items[i + CHUNK_SIZE]);
  }
  free(set);
}

unsigned char *get_md5(FILE *file) {
  unsigned char *c = malloc(sizeof(*c) * MD5_DIGEST_LENGTH);
  if (c == NULL) {
    fprintf(stderr, "[ERROR] Buy more RAM\n");
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
      char full_path[PATH_SIZE] = {0};
      strncpy(full_path, path, strlen(path));
      full_path[strlen(path)] = '/';
      strncat(full_path, de->d_name, 255);
      strcpy((char *)dp, full_path);
      traverse_dir(set, full_path);
    } else if (de->d_type == DT_REG) {
      char full_path[PATH_SIZE] = {0};
      strncpy(full_path, path, strlen(path));
      full_path[strlen(path)] = '/';
      strncat(full_path, de->d_name, 255);
      strcpy((char *)dp, full_path);
      add_file(set, full_path);
    }
  }
  closedir(dir);
}

void *main_loop(void *arg) {
  struct Arg *a = (struct Arg *)arg;
  int argc = a->argc;
  char **argv = a->argv;
  Set *set = a->set;
  while (argc > 1) {
    const char *path = argv[--argc];
    if (path[0] == '-') {
      continue;
    }
    if (is_dir(path)) {
      traverse_dir(set, path);
      continue;
    }
    add_file(set, path);
  }
  all_read = true;
  return NULL;
}

void *spinner(void *arg) {
  struct Arg *a = (struct Arg *)arg;
  int argc = a->argc;
  char **argv = a->argv;
  bool print_names = false;
  bool print_index = false;
  bool no_spinner = false;
  size_t counter = 0;
  char tdp[PATH_SIZE];
  while (argc > 1) {
    const char *v = argv[--argc];
    if (strcmp(v, "-d") == 0) {
      print_names = 1;
    }
    if (strcmp(v, "-i") == 0) {
      print_index = 1;
    }
    if (strcmp(v, "-n") == 0) {
      no_spinner = 1;
    }
  }
  do {
    if (strcmp((char *)dp, tdp) != 0) {
      strcpy(tdp, (char *)dp);
      counter++;
      fprintf(stderr, "%s", MOVE_TO_BEGINING);
      if (!no_spinner) {
        fprintf(stderr, "%c ", SPIN[counter % SPIN_SIZE]);
      }
      if (print_index) {
        fprintf(stderr, "%6zu ", counter);
      }
      if (print_names) {
        fprintf(stderr, "%s", (char*)dp);
      }
    }
  } while (!all_read);
  fprintf(stderr,"%s\n", MOVE_TO_BEGINING);
  return NULL;
}

int main(int argc, char **argv) {
  Set *set = set_init();
  fprintf(stderr, "[LOG] Reading files\n");
  struct Arg arg;
  arg.argc = argc;
  arg.argv = argv;
  arg.set = set;
  pthread_t main_tid, spinner_tid;
  pthread_create(&main_tid, NULL, main_loop, &arg);
  pthread_create(&spinner_tid, NULL, spinner, &arg);

  pthread_join(main_tid, NULL);
  pthread_join(spinner_tid, NULL);

  fprintf(stderr, "[LOG] All files read\n");
  char c;
  char fn[256] = {0};
  bool done = 0;
  FILE *f;
  do {
    printf("Command[qfdp]:");
    scanf(" %c", &c);
    switch (c) {
      case 'q':;
        done = true;
        break;
      case 'f':;
        printf("Enter file name:");
        scanf("%s", fn);
        f = fopen(fn, "wb");
        set_print_all_files(set, f);
        fclose(f);
        printf("File saved\n");
        break;
      case 'd':;
        printf("Enter file name:");
        scanf("%s", fn);
        f = fopen(fn, "wb");
        set_print_all_duplicates(set, f);
        fclose(f);
        printf("File saved\n");
        break;
      case 'p':;
        printf("Enter file name:");
        scanf("%s", fn);
        f = fopen(fn, "wb");
        set_print_all_duplicates_paths(set, f);
        fclose(f);
        printf("File saved\n");
        break;
      default:
        fprintf(stderr, "Unknwon command %c\n", c);
    }
  } while (!done);
  set_delete(set);
  return 0;
}
