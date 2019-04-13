#ifndef ioutils_h_INCLUDED
#define ioutils_h_INCLUDED

#include <stdio.h> 
#include <stdbool.h> 

int fpeek(FILE *file); 
int skip_space_fgetc(FILE *file); 
void skip_line(FILE *file); 
void copy_line(FILE* src, FILE *dst); 
bool is_eof(FILE *file);
void check(int code);

#endif // ioutils_h_INCLUDED

