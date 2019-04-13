#include "bitset.h"

#include <string.h>
#define BIT(i) (1ul << (i))

void bitset_clear(struct bitset *bs) {
    memset(bs->data, 0, sizeof(bs->data));
}

void bitset_add(struct bitset *bs, uint64_t x) {
    bs->data[x>>6] |= BIT(x & 63);
}

void bitset_rm(struct bitset *bs, uint64_t x) {
    bs->data[x>>6] &=~ BIT(x & 63);
}

bool bitset_test(struct bitset *bs, uint64_t x) {
    return !!(bs->data[x>>6] & BIT(x & 63));
}

bool bitset_subset(struct bitset *superset, struct bitset *subset) {
    for (size_t i = 0; i < BITSET_SIZE; ++i) {
        if (subset->data[i] & ~superset->data[i])
            return false;
    }
    return true;
}

void bitset_subtract(struct bitset *minuend, struct bitset *subtrahend) {
    for (size_t i = 0; i < BITSET_SIZE; ++i) {
        minuend->data[i] &= ~subtrahend->data[i];
    }
}

void bitset_sum(struct bitset *augend, struct bitset *addend) {
    for (size_t i = 0; i < BITSET_SIZE; ++i) {
        augend->data[i] |= addend->data[i];
    }
}

void bitset_print(struct bitset *bs, uint64_t offset, FILE *file) {
    fputc('(', file);
    const char *separator = "";
    for (size_t i = 0; i < sizeof(bs->data) * CHAR_BIT; ++i) {
        if (bitset_test(bs, i)) {
            fprintf(file, "%s%ld", separator, i + offset);
            separator = ", ";
        }
    }
    fputc(')', file);
}
