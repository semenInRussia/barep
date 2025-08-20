// implementation of aho corasick

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define AHO_ALPHABET 256 // support only characters from 0 to 255 (including)

typedef struct {
  // amount of words for which this node is the lasted symbol
  int cnt;

  // pointers to children or -1 what's mean that on this character edge isn't
  // exist.
  //
  // here pointer is an index in the aho_heap (TODO: use vector instead?)
  //
  // after aho_build, they will automata movements
  int to[AHO_ALPHABET];

  int link;

  // check the string of node, count all substrings that are input templates
  //
  // must be -1 if have not counted yet
  int contain_terms;
} Aho;

// maximum amount of aho-corasick nodes, it's must be equal to about the size of
// all patterns
#define AHO_HEAP_SZ 4096
Aho aho_heap[AHO_HEAP_SZ];
static int aho_heap_top = 0;

// work with indexes in aho_heap like it pointer
int aho_count(int x) { return aho_heap[x].cnt; }
int aho_link(int x) { return aho_heap[x].link; }
int aho_go(int x, size_t c) { return aho_heap[x].to[c]; }

// return "pointer" to new aho-corasick node
int aho_make() {
  int j = aho_heap_top;
  aho_heap[j].contain_terms = aho_heap[j].link = -1;
  aho_heap[j].cnt = 0;
  for (int i = 0; i < AHO_ALPHABET; i++) {
    aho_heap[j].to[i] = -1;
  }
  ++aho_heap_top;
  return j;
}

void aho_add(int x, const char *word) {
  for (int i = 0; word[i]; i++) {
    char c = word[i];
    if (aho_go(x, c) == -1) {
      aho_heap[x].to[(size_t)c] = aho_make();
    }
    x = aho_go(x, c);
  }
  ++aho_heap[x].cnt;
}

// bfs build

typedef struct {
  size_t count;
  size_t capacity;
  int *items;
  int l, r;
} Aho_Queue;

void aho_queue_reserve(Aho_Queue *q, size_t sz) {
  q->items = realloc(q->items, sz * sizeof(struct Aho *));
  q->capacity = sz;
}

int aho_queue_pop(Aho_Queue *q) {
  assert(q->count && "pop empty queue");
  int res = q->items[q->l];
  ++q->l;
  --q->count;
  return res;
}

int aho_queue_front(Aho_Queue q) {
  assert(q.count && "pop empty queue");
  return q.items[q.l];
}

void aho_queue_push(Aho_Queue *q, int x) {
  if ((size_t)q->r >= q->capacity) {
    aho_queue_reserve(q, q->capacity * 2 + 1);
  }
  q->items[q->r++] = x;
  q->count++;
}

void aho_build(int root) {
  // automata actions:
  // go(t, c) = go(link(t), c)
  // link(y) = go(link(x), c)

  Aho_Queue q = {0};
  aho_queue_push(&q, root);
  aho_heap[root].link = root;

  while (q.count > 0) {
    int x = aho_queue_pop(&q);
    for (size_t i = 0; i < AHO_ALPHABET; i++) {
      int y = aho_go(x, i);
      if (y == -1) { // go(x, c)
        aho_heap[x].to[i] = x == root ? root : aho_go(aho_link(x), i);
      } else { // link(y)
        aho_heap[y].link = x == root ? root : aho_go(aho_link(x), i);
        aho_queue_push(&q, y);
      }
    }
  }
}

// print

int top = 0;
char buf[256];

void aho_print(int x) {
  printf("%d, %s\n", x, buf);
  for (size_t c = 0; c < AHO_ALPHABET; c++) {
    if (aho_go(x, c) == -1) {
      continue;
    }
    buf[top++] = c;
    aho_print(aho_go(x, c));
    --top;
  }
}

// aho_count_inside()

int aho_count_inside(int t, int x) {
  int ans = aho_heap[x].contain_terms;
  if (ans != -1) {
    return ans;
  }
  if (x == t) {
    return 0;
  }
  ans = aho_count(x) + aho_count(aho_link(x));
  aho_heap[x].contain_terms = ans;
  return ans;
}

int main(int argc, const char *argv[]) {
  // argv[1] - text
  // argv[2], argv[3], ... - templates
  //
  // display amount of occurances of templates inside text

  assert(argc > 1);

  int t = aho_make();

  for (int i = 2; i < argc; i++) {
    aho_add(t, argv[i]);
  }

  aho_build(t);

  const char *txt = argv[1];
  int ans = 0;
  int cur = t;
  for (size_t i = 0; txt[i]; i++) {
    cur = aho_go(cur, txt[i]);
    ans += aho_count_inside(t, cur);
  }
  printf("count %d occurances of given %d templates", ans, argc - 2);
}
