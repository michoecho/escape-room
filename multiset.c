#include "multiset.h"
#include <assert.h>

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

void multiset_clear(struct multiset *ms) {
    for (int i = 0; i < MULTISET_SIZE; ++i) {
        ms->data[i] = 0;
    }
}

void multiset_add(struct multiset *ms, int x) {
    ms->data[x] += 1;
}

void multiset_rm(struct multiset *ms, int x) {
    ms->data[x] -= 1;
}

int multiset_get(struct multiset *ms, int x) {
    return ms->data[x];
}

bool multiset_subset(struct multiset *superset, struct multiset *subset) {
    for (int i = 0; i < MULTISET_SIZE; ++i) {
        if (superset->data[i] < subset->data[i]) {
            return false;
        }
    }
    return true;
}

void multiset_subtract(struct multiset *minuend, struct multiset *subtrahend) {
    assert(multiset_subset(minuend, subtrahend));
    for (int i = 0; i < MULTISET_SIZE; ++i) {
        minuend->data[i] -= subtrahend->data[i];
    }
}

void multiset_sum(struct multiset *augend, struct multiset *addend) {
    for (int i = 0; i < MULTISET_SIZE; ++i) {
        augend->data[i] = MAX(augend->data[i], addend->data[i]);
    }
}

void multiset_print(struct multiset *ms, int offset, FILE *file) {
    fputc('(', file);
    const char *separator = "";
    for (int i = 0; i < MULTISET_SIZE; ++i) {
        if (ms->data[i] > 0) {
            fprintf(file, "%s%d=%d", separator, i + offset, ms->data[i]);
            separator = ", ";
        }
    }
    fputc(')', file);
}
