// NOTE that documentation is located inside an aho.c file

#ifndef _AHO_H_INCLUDED_
#define _AHO_H_INCLUDED_

#include <stdbool.h>
#include <stdint.h>

// Pointer to a node of an aho-corasick tree or -1 that can be considered like
// NULL.  Here "pointer" is an index in the aho_heap
typedef int Aho_Node_Ptr;

// each node of aho corasick tree can be represented be any string.  To be
// simpler I will be consider string and node as the same.

#define AHO_ALPHABET 256 // support only characters from 0 to 255 (including)

typedef struct Aho {
  // if this state is matching, `pattern` is equal to the index of this pattern
  // The index of a pattern is specified when you add it via `aho_add()`.  If
  // this node isn't matching `pattern` is -1
  int pattern;

  // check the string of node, count all substrings that are inside "dict"
  //
  // you can use aho_matches_count to get this value by pointer
  //
  // if you are iterating through text char by char, going from state to state
  // with `aho_go(t, c)`, then you can sum all states `aho_matches_count` and
  // get the amount of occurances of "dict"'s words
  int cnt_matches;

  // if match_size=0, then current state don't contain "dict"'s words, otherwise
  // match_size indicates the size of the longest match at the current state.
  //
  // you can use `aho_match_size` to get this value by pointer
  //
  // NOTE that if you need to check matches use `aho_next_match()` (check
  // above).  Use `aho_match_size` only if you need check the biggiest match at
  // this node
  //
  // if you are iterating through text char by char, going from state to state
  // with `aho_go(t, c)`, you can get match size and check the found string
  // because you know the end and the beginning (end-string size).
  int match_size;

  // convinient way to use states like Aho_t is to iterate through all matches
  // these state catch.  You can do it using aho_next_match(t) which return the
  // point to one of the "sub-nodes" which are nodes of strings added into dict
  //
  // if node doesn't contain these matches return root state.
  //
  // solution is when you have t - the current state iterate through all
  // aho_count_matches() matches using while-cycle like the following:
  //
  //     for (Aho_Node_Ptr x = cur; aho_size(x) > 0; x = aho_next_match(x))
  //         if (aho_is_match(x))
  //             // here a match
  int nxt_match;

  // the length of node's string
  int size;

  // after `aho_build`, they will be finite-state machine transitions, before
  // to[c] is pointer to children in default trie
  Aho_Node_Ptr to[AHO_ALPHABET];
  Aho_Node_Ptr link;
} Aho_t;

// queue:
#define AHO_HEAP_SZ 4096
#define AHO_QUEUE_DEFAULT_CAPACITY 2

typedef struct {
  size_t count;
  size_t capacity;
  Aho_Node_Ptr *items;
  int l, r;
} Aho_Queue;

// structs:
typedef int Aho_Node_Ptr;

// you can compute&construct states using the code like the following:
//
//     Aho_Node_Ptr t = aho_make();
//     aho_add("<template1>");
//     aho_add("<template2>");
//     ...
//     aho_build();

Aho_Node_Ptr aho_make();
void aho_add(Aho_Node_Ptr x, const char *word, int idx);
void aho_build(Aho_Node_Ptr root);

// Now you can starting from the empty node (you have already computed it).
// And update the state using `aho_go()`
Aho_Node_Ptr aho_go(Aho_Node_Ptr x, size_t c);

// if the current state have a match, `aho_is_match()` return true
bool aho_is_match(Aho_Node_Ptr x);

// if the current state is starting return true.
//
// in this case you can use anything like memchr to skip text until the first
// template symbol
bool aho_is_start(Aho_Node_Ptr x);

// `aho_matches_count()` returns amount of matching at the current state
int aho_matches_count(Aho_Node_Ptr x);

// you can starting from the current state check every next match until it isn't
// the root (aho_size() == 0) jumping through all matches using
// `aho_next_match()`.
int aho_size(Aho_Node_Ptr x);
int aho_match_size(Aho_Node_Ptr x);
Aho_Node_Ptr aho_next_match(Aho_Node_Ptr x);

// also using function `aho_iter()` you can skip the current state if it isn't a
// match.  It is useful when you need to iterate through all matches of current
// state (including overlap matches)
Aho_Node_Ptr aho_match_iter(Aho_Node_Ptr x);

// using all these functions you can print all occurances and positions where it
// occured.
//
//     const char *src = ...;
//     Aho_Node_Ptr dict = ...;
//     Aho_Node_Ptr t = dict;
//     for (int i = 0; src[i]; i++) {
//       t = aho_go(t, src[i]);
//       for (Aho_Node_Ptr x = aho_match_iter(t); aho_size(x); x =
//            aho_next_match(x)) {
//         int sz = aho_size(x);
//         // [i-sz+1; i] - is a match
//         printf("%d: %.*s", i, sz, &src[i + 1 - sz]);
//       }
//     }

// also the `aho_for_match()` macro is useful to iterate through all iterations
// it accept variable name and the current state
#define aho_for_match(x, t)                                                    \
  for (Aho_Node_Ptr x = aho_match_iter(t); aho_size(x) > 0;                    \
       x = aho_next_match(x))

#endif // _AHO_H_INCLUDED_
