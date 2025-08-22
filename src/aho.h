// NOTE that documentation is located inside an aho.c file

#include <stdbool.h>

// structs:
typedef int Aho_Node_Ptr;

// init:
Aho_Node_Ptr aho_make();
void aho_add(Aho_Node_Ptr x, const char *word);
void aho_build(Aho_Node_Ptr root);

// use:
Aho_Node_Ptr aho_next_match(Aho_Node_Ptr x);
int aho_matches_count(Aho_Node_Ptr x);
bool aho_is_match(Aho_Node_Ptr x);
int aho_match_size(Aho_Node_Ptr x);

#define AHO_ALPHABET 1000 // support only characters from 0 to 255 (including)

#ifndef AHO_H_
#define AHO_H_
#include "aho.c"
#endif
