#include "ioutils.h"
#include <stdlib.h>

int fpeek(FILE *file) {
    int c = fgetc(file);
    ungetc(c, file);
    return c;
}

int skip_space_fgetc(FILE *file) {
    int c;
    do c = fgetc(file); while (c == ' '); 
    return c;
}

void skip_line(FILE *file) {
    int c;
    do c = fgetc(file); while (c != '\n' && c != EOF);
}

void copy_line(FILE* src, FILE *dst) {
    int c = fgetc(src);
    while (c != '\n' && c != EOF) {
        fputc(c, dst);
        c = fgetc(src);
    }
}

bool is_eof(FILE *file) {
    return fpeek(file) == EOF;
}

void check(int code) {
    if (code < 0) {
        perror("FATAL");
        exit(EXIT_FAILURE);
    }
}
