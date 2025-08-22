// implementation of aho corasick

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define AHO_ALPHABET 1000 // support only characters from 0 to 255 (including)

// Pointer to a node of an aho-corasick tree or -1 that can be considered like
// NULL.  Here "pointer" is an index in the aho_heap
typedef int Aho_Node_Ptr;

// each node of aho corasick tree can be represented be any string.  To be
// simpler I will be consider string and node as the same.

typedef struct Aho {
  // amount of templates in aho-corasick tree that's equal to node's string.
  // these nodes are called terminal nodes, so i named the variable `term`
  int term;

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
  //     for (Aho_Node_Ptr x = cur; aho_size(x) > 0; x = aho_next_match(x)) {
  //       if (aho_is_match(x)) {
  //         // here a match
  //       }
  //     }
  int nxt_match;

  // the length of node's string
  int size;

  // after `aho_build`, they will be finite-state machine transitions, before
  // to[c] is pointer to children in default trie
  Aho_Node_Ptr to[AHO_ALPHABET];
  Aho_Node_Ptr link;
} Aho_t;

// maximum amount of aho-corasick nodes, it's must be equal to about the size of
// all patterns
#define AHO_HEAP_SZ 4096
Aho_t aho_heap[AHO_HEAP_SZ]; // TODO: use vector instead?
static int aho_heap_top = 0;

// work with indexes in aho_heap like it pointer (type Aho_Node_Ptr)

int aho_match_size(Aho_Node_Ptr x) { return aho_heap[x].match_size; }
int aho_matches_count(Aho_Node_Ptr x) { return aho_heap[x].cnt_matches; }
Aho_Node_Ptr aho_next_match(Aho_Node_Ptr x) { return aho_heap[x].nxt_match; }
Aho_Node_Ptr aho_go(Aho_Node_Ptr x, size_t c) { return aho_heap[x].to[c]; }

Aho_Node_Ptr aho_link(Aho_Node_Ptr x) { return aho_heap[x].link; }
int aho_size(Aho_Node_Ptr x) { return aho_heap[x].size; }
bool aho_is_match(Aho_Node_Ptr x) { return aho_heap[x].term > 0; }
int aho_term(Aho_Node_Ptr x) { return aho_heap[x].term; }

// return "pointer" to the new aho-corasick node
Aho_Node_Ptr aho_make() {
  Aho_Node_Ptr j = aho_heap_top;
  ++aho_heap_top;
  aho_heap[j].link = -1;
  aho_heap[j].cnt_matches = aho_heap[j].term = 0;
  for (int i = 0; i < AHO_ALPHABET; i++) {
    aho_heap[j].to[i] = -1;
  }
  return j;
}

void aho_add(Aho_Node_Ptr x, const char *word) {
  for (int i = 0; word[i]; i++) {
    char c = word[i];
    if (aho_go(x, c) == -1) {
      aho_heap[x].to[(size_t)c] = aho_make();
    }
    x = aho_go(x, c);
  }
  ++aho_heap[x].term;
}

// bfs build

typedef struct {
  size_t count;
  size_t capacity;
  Aho_Node_Ptr *items;
  int l, r;
} Aho_Queue;

void aho_queue_reserve(Aho_Queue *q, size_t sz) {
  q->items = (Aho_Node_Ptr *)realloc(q->items, sz * sizeof(Aho_Node_Ptr));
  q->capacity = sz;
}

Aho_Node_Ptr aho_queue_pop(Aho_Queue *q) {
  assert(q->count && "pop empty queue");
  int res = q->items[q->l];
  ++q->l;
  --q->count;
  return res;
}

#define AHO_QUEUE_DEFAULT_CAPACITY 2

void aho_queue_push(Aho_Queue *q, Aho_Node_Ptr x) {
  if (q->capacity == 0) {
    aho_queue_reserve(q, AHO_QUEUE_DEFAULT_CAPACITY);
  }
  while ((size_t)q->r >= q->capacity) {
    aho_queue_reserve(q, q->capacity * 2 + 1);
  }
  q->items[q->r++] = x;
  q->count++;
}

void aho_build(Aho_Node_Ptr root) {
  // transitions:
  // go(t, c) = go(link(t), c)
  // link(y) = go(link(x), c)

  Aho_Queue q = {0};
  aho_queue_push(&q, root);
  aho_heap[root].nxt_match = aho_heap[root].link = root;
  aho_heap[root].size = aho_heap[root].match_size = 0;

  while (q.count > 0) {
    Aho_Node_Ptr x = aho_queue_pop(&q);
    for (size_t i = 0; i < AHO_ALPHABET; i++) {
      Aho_Node_Ptr y = aho_go(x, i);
      if (y == -1) { // go(x, c)
        aho_heap[x].to[i] = x == root ? root : aho_go(aho_link(x), i);
        continue;
      }
      // link(y)
      int lnk = x == root ? root : aho_go(aho_link(x), i);
      Aho_t *t = &aho_heap[y];
      t->link = lnk;
      t->size = aho_size(x) + 1;
      t->cnt_matches = aho_matches_count(lnk) + aho_term(y);
      t->match_size = aho_term(y) ? aho_size(y) : aho_match_size(lnk);
      t->nxt_match = aho_term(lnk) ? lnk : aho_next_match(lnk);
      aho_queue_push(&q, y);
    }
  }
}
