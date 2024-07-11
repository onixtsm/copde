#include "set.h"

#include <linux/limits.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./md5.h"
#include "item.h"

#define CHUNK_SIZE 1024

Set *set_init(void) {
  Set *set = malloc(sizeof(*set) * CHUNK_SIZE);
  if (set == NULL) {
    fprintf(stderr, "[ERROR] Buy more RAM\n");
    return NULL;
  }
  set->capacity = CHUNK_SIZE;
  set->items = malloc(sizeof(set->items) * CHUNK_SIZE);
  set->size = 0;
  return set;
}

void set_add(Set *set, MD5_HASH hash, const char *path) {
  if (set->capacity == set->size) {
    set->items = reallocarray(set->items, set->capacity + CHUNK_SIZE, sizeof(set->items));
    set->capacity += CHUNK_SIZE;
  }
  Item *item = item_new(hash);
  item_add_path(item, path);
  set->items[set->size++] = item;
}

void set_combine_items(Set *set) {
  char tpath[PATH_MAX] = {0};
  Item *titem = set->items[0];
  for (size_t i = 1; i < set->size; ++i) {
    if (strcmp((char *)titem->hash, (char *)set->items[i]->hash) == 0) {
      item_add_path(titem, *set->items[i]->paths);
    } else {
      titem = set->items[i];
    }
  }
}

void set_delete(Set *set) {
  for (size_t i = 0; i < set->capacity; ++i) {
    fprintf(stderr, "\r%zu of %zu", i, set->capacity);
    if (i < set->size) {
      item_delete(set->items[i]);
    }
  }
  free(set->items);
  set->items = NULL;
  free(set);
}
int compar(const void *i1, const void *i2) {
  Item *ti1 = (Item *)i1;
  Item *ti2 = (Item *)i2;
  for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    if (ti2[i].hash - ti1[i].hash != 0) {
      return ti2[i].hash - ti1[i].hash;
    }
  }
  return 0;
}

void set_sort(Set *set) { qsort(set->items, set->size, sizeof(set->items), compar); }

void set_cat(Set *set1, Set *set2) {
  if (set1 == NULL || set2 == NULL) {
    return;
  }
  int needed_size = set1->size + set2->size - set1->capacity;
  if (needed_size > 0) {
    set1->items = reallocarray(set1->items, set1->capacity + needed_size, sizeof(set1->items));
    set1->capacity = set1->capacity + needed_size;
  }
  for (size_t i = 0; i < set2->size; ++i) {
    set1->items[i + set1->size] = set2->items[i];
  }
  set1->size += set2->size;
  set2->size = 0;
  free(set2->items);
  set2->items = NULL;
  free(set2);
}

void set_print(Set *set) {
  for (size_t i = 0; i < set->size; ++i) {
    md5_print(set->items[i]->hash);
    for (size_t j = 0; j < set->items[i]->paths_count; ++j) {
      fprintf(stderr, " %s", set->items[i]->paths[j]);
    }
    putc('\n', stderr);
  }
}

void set_print_duplicates(Set *set) {
  for (size_t i = 0; i < set->size; ++i) {
    if (set->items[i]->paths_count > 1) {
      md5_print(set->items[i]->hash);
      for (size_t j = 0; j < set->items[i]->paths_count; ++j) {
        fprintf(stderr, " %s", set->items[i]->paths[j]);
      }
      putc('\n', stderr);
    }
  }
}
