// structs:
typedef int Aho_Node_Ptr;

// init:
Aho_Node_Ptr aho_make();
void aho_add(Aho_Node_Ptr x, const char *word);
void aho_build(Aho_Node_Ptr root);

// use:
int aho_count(Aho_Node_Ptr x);

#ifndef AHO_H_
#define AHO_H_
#include "aho.c"
#endif
