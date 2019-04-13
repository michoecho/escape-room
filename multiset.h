#ifndef multiset_h_INCLUDED
#define multiset_h_INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define MULTISET_SIZE ('Z' - 'A' + 1)

struct multiset {
    int data[MULTISET_SIZE];
};

void multiset_clear(struct multiset *ms);
void multiset_add(struct multiset *ms, int x);
void multiset_rm(struct multiset *ms, int x);
int multiset_get(struct multiset *ms, int x);
bool multiset_subset(struct multiset *superset, struct multiset *subset);
void multiset_subtract(struct multiset *minuend, struct multiset *subtrahend);
void multiset_sum(struct multiset *augend, struct multiset *addend);
void multiset_print(struct multiset *ms, int offset, FILE *file);

#endif // multiset_h_INCLUDED

