#ifndef ITEM_H_
#define ITEM_H_
#include "./md5.h"
#include <stdbool.h>

typedef struct {
  MD5_HASH hash;
  char **paths;
  size_t paths_count;
} Item;

Item *item_new(MD5_HASH hash);
bool item_add_path(Item *item, const char *path);
void item_delete(Item *item);

#endif
