// implementation of aho corasick

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define AHO_ALPHABET 1000 // support only characters from 0 to 255 (including)

// Pointer to a node of an aho-corasick tree or -1 that can be considered like
// NULL.  Here "pointer" is an index in the aho_heap
typedef int Aho_Node_Ptr;

// each node of aho corasick tree can be represented be any string.  To be
// simpler I will be consider string and node as the same.

typedef struct {
  // amount of templates in aho-corasick tree that's equal to node's string.
  // these nodes are called terminal, so i named variable `term`
  int term;

  // after `aho_build`, they will be finite-state machine transitions, before
  // to[c] is pointer to children in default trie
  Aho_Node_Ptr to[AHO_ALPHABET];
  Aho_Node_Ptr link;

  // check the string of node, count all substrings that are input templates
  int inside_term; // must be -1 if it have not computed yet
} Aho;

// maximum amount of aho-corasick nodes, it's must be equal to about the size of
// all patterns
#define AHO_HEAP_SZ 4096
Aho aho_heap[AHO_HEAP_SZ]; // TODO: use vector instead?
static int aho_heap_top = 0;

// work with indexes in aho_heap like it pointer (type Aho_Node_Ptr)

int aho_term(Aho_Node_Ptr x) { return aho_heap[x].term; }
Aho_Node_Ptr aho_link(Aho_Node_Ptr x) { return aho_heap[x].link; }
Aho_Node_Ptr aho_go(Aho_Node_Ptr x, size_t c) { return aho_heap[x].to[c]; }

// return "pointer" to the new aho-corasick node
Aho_Node_Ptr aho_make() {
  Aho_Node_Ptr j = aho_heap_top;
  ++aho_heap_top;
  aho_heap[j].inside_term = aho_heap[j].link = -1;
  aho_heap[j].term = 0;
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
  q->items = realloc(q->items, sz * sizeof(struct Aho *));
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
  if ((size_t)q->r >= q->capacity) {
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
  aho_heap[root].link = root;

  while (q.count > 0) {
    Aho_Node_Ptr x = aho_queue_pop(&q);
    for (size_t i = 0; i < AHO_ALPHABET; i++) {
      Aho_Node_Ptr y = aho_go(x, i);
      if (y == -1) { // go(x, c)
        aho_heap[x].to[i] = x == root ? root : aho_go(aho_link(x), i);
      } else { // link(y)
        aho_heap[y].link = x == root ? root : aho_go(aho_link(x), i);
        aho_queue_push(&q, y);
      }
    }
  }
}

// aho_count()

int aho_count(Aho_Node_Ptr t, Aho_Node_Ptr x) {
  int ans = aho_heap[x].inside_term;
  if (ans != -1) {
    return ans;
  }
  if (x == t) {
    return 0;
  }
  aho_heap[x].inside_term = aho_term(x) + aho_term(aho_link(x));
  return aho_heap[x].inside_term;
}
