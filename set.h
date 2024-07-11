#ifndef SET_H_
#define SET_H_
#include <stdlib.h>
#include <stdio.h>
#include "item.h"

typedef struct {
  Item **items;
  size_t size;
  size_t capacity;
} Set;

Set *set_init(void);
void set_expand(Set *set);
void set_add(Set *set, unsigned char *key, const char *path);
void set_delete(Set *set);
void set_sort(Set *set);
void set_print(Set *set);
void set_combine_items(Set *set);
void set_print_duplicates(Set *set);
void set_cat(Set *set1, Set *set2);

#endif
