#ifndef bitset_h_INCLUDED
#define bitset_h_INCLUDED

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define BITSET_BITS 1024
#define BITSET_SIZE (BITSET_BITS + 63 / 64)

struct bitset {
    uint64_t data[BITSET_SIZE];
};

void bitset_clear(struct bitset *bs);
void bitset_add(struct bitset *bs, uint64_t x);
void bitset_rm(struct bitset *bs, uint64_t x);
bool bitset_test(struct bitset *bs, uint64_t x);
bool bitset_subset(struct bitset *superset, struct bitset *subset);
void bitset_subtract(struct bitset *minuend, struct bitset *subtrahend);
void bitset_sum(struct bitset *augend, struct bitset *addend);
void bitset_print(struct bitset *bs, uint64_t offset, FILE *file);

#endif // bitset_h_INCLUDED
