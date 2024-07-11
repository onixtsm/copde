#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <openssl/md5.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "./md5.h"
#include "./set.h"

#define CHUNK_SIZE 1024
#define PATHS_MAX 20
#define THREAD_COUNT 4

bool all_read = false;
bool no_spinner = false;
bool print_names = false;
bool print_index = false;

typedef struct {
  bool free;
  pthread_t tid;
  size_t id;
  Set *set;
  bool was_used;
} Thread;

atomic_char dp[PATH_MAX];
atomic_size_t count = 0;

typedef struct {
  char path[PATH_MAX];
  Thread *thread;
} Thread_args;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

const char SPIN[] = "┤ ┘ ┴ └ ├ ┌ ┬ ┐";
const size_t SPIN_SIZE = sizeof(SPIN) / sizeof(SPIN[0]);
const char MOVE_TO_BEGINING[] = "\r";

bool is_dir(const char *path) {
  int fs = open(path, O_RDONLY);
  struct stat s;
  fstat(fs, &s);
  return S_ISDIR(s.st_mode);
}

void *add_file(void *a) {
  Thread_args *arg = (Thread_args *)a;
  char *path = arg->path;
  Thread *thread = arg->thread;
  thread->free = false;
  thread->was_used = true;
  Set *set = thread->set;
  // printf("On thread %zu, file %s\n", thread->id, path);

  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "[ERROR] Could not open file %s, because of %s\n", path, strerror(errno));
    goto END;
  }
  unsigned char *c = md5_get(f);
  pthread_mutex_lock(&lock);
  set_add(set, c, path);
  pthread_mutex_unlock(&lock);
  free(c);
  fclose(f);
END:
  free(a);
  pthread_mutex_lock(&lock);
  thread->free = true;
  pthread_mutex_unlock(&lock);
  return NULL;
}

void traverse_dir(Thread *threads, const char *path) {
  struct dirent *de;
  DIR *dir = opendir(path);
  if (dir == NULL) {
    fprintf(stderr, "Could not open dir %s, because of %s\n", path, strerror(errno));
    return;
  }
  while ((de = readdir(dir)) != NULL) {
    char full_path[PATH_MAX] = {0};
    sprintf(full_path, "%s/%s", path, de->d_name);
    strcpy((char *)dp, full_path);
    if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
      traverse_dir(threads, full_path);
    } else if (de->d_type == DT_REG) {
      for (size_t i = 0; 1; ++i) {
        pthread_mutex_lock(&lock);
        i = i % THREAD_COUNT;
        if (threads[i].free) {
          Thread_args *a = malloc(sizeof(*a));
          if (a == NULL) {
            fprintf(stderr, "Buy more RAM\n");
            break;
          }
          strcpy(a->path, full_path);
          a->thread = &threads[i];

          pthread_mutex_unlock(&lock);
          // Thread_args a = {full_path, &threads[i]};
          pthread_create(&threads[i].tid, NULL, add_file, a);
          break;
        }
        pthread_mutex_unlock(&lock);
      }
    }
  }
  closedir(dir);
}

void main_loop(Thread *threads, char *path_collection) {
  char path[PATH_MAX] = {0};
  while (0 != *strcpy(path, path_collection)) {
    if (is_dir(path)) {
      traverse_dir(threads, path);
    } else {
      Thread_args *a = malloc(sizeof(*a));
      if (a == NULL) {
        continue;
      }
      strcpy(a->path, path);
      a->thread = &threads[0];
      // Thread_args a = {path, &threads[0]};
      add_file(a);
    }
    path_collection += strlen(path) + 1;
    memset(path, 0, PATH_MAX);
  }
  all_read = true;
}
void *spinner(void *arg) {
  Thread_args *a = (Thread_args *)arg;
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
        fprintf(stderr, "%6zu ", counter);  // Counter shows x2 values for some reason;
      }
      if (print_names) {
        fprintf(stderr, "%s", (char *)dp);
      }
    }
  } while (!all_read);
  fprintf(stderr, "%s\n", MOVE_TO_BEGINING);
  return NULL;
}

void print_help(void) { assert(false && "Not implementded"); }

char *parse_arguments(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage copde [flags] <dir|file> [dir|file] ...\n");
    return NULL;
  }
  char *path_collection =
      malloc(sizeof(*path_collection) * PATHS_MAX * PATH_MAX);  // Space fo 20 PATH_MAX, every path is 4096 bytes

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

void *set_sort_wrapper(void *s) {
  set_sort((Set *)s);
  return NULL;
}

void sort_threaded_sets(Thread *threads) {
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    pthread_create(&threads[i].tid, NULL, set_sort_wrapper, threads->set);
  }
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    pthread_join(threads[i].tid, NULL);
  }
}

int main(int argc, char **argv) {
  char *path_collection = parse_arguments(argc, argv);
  if (path_collection == NULL) {
    return 1;
  }

  fprintf(stderr, "[LOG] Reading files\n");

  pthread_t main_tid, spinner_tid;
  Thread_args arg;
  Thread threads[THREAD_COUNT];
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    threads[i].free = true;
    threads[i].id = i;
    threads[i].set = set_init();
  }
  strcpy(arg.path, path_collection);  // POSSIBLE OUT OF MEMORY
  // arg.path = path_collection;
  arg.thread = threads;
  pthread_create(&spinner_tid, NULL, spinner, &arg);
  main_loop(threads, path_collection);

  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    if (threads[i].was_used) {
      pthread_join(threads[i].tid, NULL);
    }
  }
  pthread_join(spinner_tid, NULL);
  free(path_collection);
  sort_threaded_sets(threads);
  Set *set = threads[0].set;
  for (size_t i = 1; i < THREAD_COUNT; ++i) {
    set_cat(set, threads[i].set);
  }
  fprintf(stderr, "[LOG] %zu files read\n", set->size);
  // set_sort(set);
  fprintf(stderr, "===================================\n");
  // set_combine_items(set);
  fprintf(stderr, "===================================\n");
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
