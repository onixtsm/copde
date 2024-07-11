#include "./item.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"

Item *item_new(MD5_HASH hash) {
  Item *item = malloc(sizeof(*item));
  if (item == NULL) {
    fprintf(stderr, "[ERROR] Buy more RAM!\n");
    return NULL;
  }
  md5_copy_hash(item->hash, hash);
  item->paths = NULL;
  item->paths_count = 0;
  return item;
}

void item_delete(Item *item) {
  if (item == NULL) {
    return;
  }
  if (item->paths != NULL) {
    for (size_t i = 0; i < item->paths_count; ++i) {
      free(item->paths[i]);
    }
  }
  free(item->paths);
  item->paths = NULL;
  free(item);
}

bool item_add_path(Item *item, const char *path) {
  if (item->paths == NULL) {
    item->paths = malloc(sizeof(item->paths));
    if (item->paths == NULL) {
      fprintf(stderr, "[ERROR] Buy more RAM!\n");
      return true;
    }
  } else {
    item->paths = reallocarray(item->paths, item->paths_count + 1, sizeof(item->paths));
  }
  size_t path_len = strlen(path);
  item->paths[item->paths_count] = malloc(sizeof(item->paths[item->paths_count]) * (path_len + 1));
  if (item->paths[item->paths_count] == NULL) {
    fprintf(stderr, "[ERROR] Buy more RAM!\n");
    return true;
  }
  strcpy(item->paths[item->paths_count], path);
  item->paths[item->paths_count][path_len] = 0;
  item->paths_count++;
  return false;
}
