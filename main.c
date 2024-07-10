#include <assert.h>ma
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "./md5.h"
#include "./set.h"

#define CHUNK_SIZE 1024
#define PATHS_MAX 20

bool all_read = false;
bool no_spinner = false;
bool print_names = false;
bool print_index = false;

typedef struct {
  Set *set;
  char *paths_collection;
} Thread_args;

const char SPIN[] = "┤ ┘ ┴ └ ├ ┌ ┬ ┐";
const size_t SPIN_SIZE = sizeof(SPIN) / sizeof(SPIN[0]);
const char MOVE_TO_BEGINING[] = "\r";

bool is_dir(const char *path) {
  int fs = open(path, O_RDONLY);
  struct stat s;
  fstat(fs, &s);
  return S_ISDIR(s.st_mode);
}

void add_file(Set *set, const char *path) {
  FILE *f = fopen(path, "rb");
  unsigned char *c = md5_get(f);
  set_add(set, c, path);
  free(c);
  fclose(f);
}

void traverse_dir(Set *set, const char *path) {
  struct dirent *de;
  DIR *dir = opendir(path);
  if (dir == NULL) {
    fprintf(stderr, "Could not open dir %s, because of %s\n", path, strerror(errno));
    return;
  }
  while ((de = readdir(dir)) != NULL) {
    if (de->d_type == DT_DIR && de->d_name[0] != '.') {
      char full_path[PATH_MAX] = {0};
      sprintf(full_path, "%s/%s", path, de->d_name);
      traverse_dir(set, full_path);
    } else if (de->d_type == DT_REG) {
      char full_path[PATH_MAX] = {0};
      sprintf(full_path, "%s/%s", path, de->d_name);
      add_file(set, full_path);
    }
  }
  closedir(dir);
}

void *main_loop(void *arg) {
  Thread_args *a = (Thread_args *)arg;
  Set *set = a->set;
  char *path_collection = a->paths_collection;
  char path[PATH_MAX] = {0};
  while (0 != *strcpy(path, path_collection)) {
    if (is_dir(path)) {
      traverse_dir(set, path);
    } else {
      add_file(set, path);
    }
    path_collection += strlen(path) + 1;
    memset(path, 0, PATH_MAX);
  }
  all_read = true;
  return NULL;
}
/*
void *spinner(void *arg) {
  struct Arg *a = (struct Arg *)arg;
  size_t counter = 0;
  char tdp[PATH_MAX];
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
        fprintf(stderr, "%s", (char *)dp);
      }
    }
  } while (!all_read);
  fprintf(stderr, "%s\n", MOVE_TO_BEGINING);
  return NULL;
}
*/

void print_help(void) { assert(false && "Not implementded"); }

char *parse_arguments(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage copde [flags] <dir|file> [dir|file]\n");
    return NULL;
  }
  char *path_collection =
      malloc(sizeof(*path_collection) * PATHS_MAX * PATH_MAX);  // Space fo 20 PATH_MAX, every path is 4096 bytes
                                                                printf("%zu\n", sizeof(*path_collection) * PATHS_MAX * PATH_MAX);

  if (path_collection == NULL) {
    fprintf(stderr, "[ERROR] Buy more RAM!\n");
    return NULL;
  }
  memset(path_collection, 0, sizeof(*path_collection) * PATHS_MAX * PATH_MAX);
  size_t len = 0;
  char *a;
  for (size_t i = 1; i < argc; ++i) {
    a = argv[i];
    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      print_help();
      return NULL;
    }
    if (strcmp(a, "-d") == 0) {
      print_names = 1;
    } else if (strcmp(a, "-i") == 0) {
      print_index = 1;
    } else if (strcmp(a, "-n") == 0) {
      no_spinner = 1;
    } else {
      strcpy(path_collection + len, a);
      len += strlen(a);
      path_collection[len++] = 0;
    }
  }
  return path_collection;
}

int main(int argc, char **argv) {
  char *path_collection = parse_arguments(argc, argv);
  if (path_collection == NULL) {
    return 1;
  }
  Set *set = set_init();

  fprintf(stderr, "[LOG] Reading files\n");

  pthread_t main_tid, spinner_tid;
  Thread_args arg;
  arg.set = set;
  arg.paths_collection = path_collection;

  // pthread_create(&main_tid, NULL, main_loop, &arg);
  // pthread_create(&spinner_tid, NULL, spinner, &arg);

  // pthread_join(main_tid, NULL);
  main_loop((void *)&arg);
  // pthread_join(spinner_tid, NULL);
  free(path_collection);

  fprintf(stderr, "[LOG] All files read\n");
  set_print(set);
  set_sort(set);
  fprintf(stderr, "===================================\n");
  set_print(set);
  set_delete(set);
  return 0;
  /*
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
  */
  return 0;
}
