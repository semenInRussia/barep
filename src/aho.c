// implementation of aho corasick

#include "aho.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// maximum amount of aho-corasick nodes, it's must be equal to about the size of
// all patterns
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
