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
    if (strcmp((char *)titem->hash, (char *)set->items[i]->hash)) {
      item_add_path(titem, *set->items[i]->paths);
      // item_delete(set->items[i]);
    } else {
      titem = set->items[i];
    }
  }
}

void set_delete(Set *set) {
  for (size_t i = 0; i < set->capacity; ++i) {
    if (i < set->size) {
      item_delete(set->items[i]);
    }
    // if (set->capacity % i == CHUNK_SIZE) {
    //   free(set->items[i]);
    // }
  }
  free(set);
}
int compar(const void *i1, const void *i2) {
  return strcmp((const char *)(((Item *)i1)->hash), (const char *)(((Item *)i2)->hash));
}

void set_sort(Set *set) { qsort(set->items, set->size, sizeof(set->items), compar); }

void set_print(Set *set) {
  for (size_t i = 0; i < set->size; ++i) {
    md5_print(set->items[i]->hash);
    for (size_t j = 0; j < set->items[i]->paths_count; ++j) {
      fprintf(stderr, " %s", set->items[i]->paths[j]);
    }
    putc('\n', stderr);
  }
}
