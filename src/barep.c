#include <assert.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aho.h"

// defines

// a string that will be displayed after a file processed
#define BAREP_FILE_DIVIDER "\n"

// structs

typedef struct {
  size_t capacity;
  size_t count;
  const char **items;
} Barep_Strings;

typedef struct {
  // show or don't show:
  bool show_line_numbers;
  bool show_filenames;

  // input:
  Barep_Strings input;

  // patterns:
  Barep_Strings p;

  // if true, only count occurances (don't display lines)
  bool only_count;

  // if true, print help and exit
  bool help;
} Barep_Params;

typedef struct {
  Aho_Node_Ptr dict;
  Barep_Params params;
  size_t count; // keep amount of processed occurances
} Barep_State;

// log functions

#define barep_error(fmt, ...) fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__)

// ...

char *barep_owned(const char *s) {
  size_t len = strlen(s);
  // size_t sz = sizeof(*s) * (len + 1);
  char *out = malloc(sizeof(s[0]) * (len + 1));
  memcpy(out, s, len + 1);
  return out;
}

// vector of strings

void barep_strings_reserve(Barep_Strings *p, size_t n) {
  if (n > p->capacity) {
    p->capacity = n;
    p->items = realloc(p->items, n * sizeof(const char *));
  }
}

void barep_strings_push(Barep_Strings *p, const char *s) {
  if (p->count == 0) {
    barep_strings_reserve(p, 2);
  }
  while (p->count >= p->capacity) {
    barep_strings_reserve(p, p->capacity * 2);
  }
  p->items[p->count] = s;
  p->count++;
}

void barep_strings_free(Barep_Strings p) { free(p.items); }

// parse command line arguments

Barep_Params barep_params_default() {
  Barep_Params p = {0};

  p.show_line_numbers = true;
  p.show_filenames = true;

  // defaults to empty ones
  // p.input = ;
  // p.p = ;

  p.only_count = false;
  p.help = false;

  return p;
}

void barep_params_free(Barep_Params p) {
  barep_strings_free(p.input);
  barep_strings_free(p.p);
}

#define barep_shift(argc, argv) (assert(argc > 0), --argc, ++argv)

// barep PATH [OPTIONS] [[-e] PATTERNS ...]
int barep_parse_args(int argc, const char **argv, Barep_Params *p) {
  barep_shift(argc, argv);

  while (argc > 0) {
    const char *arg = argv[0];
    size_t arg_sz = strlen(arg);
    barep_shift(argc, argv);

    if (arg[0] == '-') {
      bool nxt = false;
      for (size_t i = 1; i < arg_sz; i++) {
        switch (arg[i]) {
        case 'h': {
          p->help = true;
          return 0;
        } break;
        // add pattern, even if it starts with -
        case 'e': {
          if (nxt) {
            barep_error("two letters in - flag require in the same argument\n");
            return 1;
          }
          if (argc == 0) {
            barep_error("-e require pattern\n");
            return 1;
          }
          nxt = true;
          barep_strings_push(&p->p, barep_owned(argv[0]));
          barep_shift(argc, argv);
        } break;
        // add a file/directory to input
        case 'f': {
          if (nxt) {
            barep_error("two letters in - flag require in the same argument\n");
            return 1;
          }
          if (argc == 0) {
            barep_error("-f require filename\n");
            return 1;
          }
          nxt = true;
          barep_strings_push(&p->input, barep_owned(argv[0]));
          barep_shift(argc, argv);
        } break;
        // don't show files
        case 'q': {
          p->show_filenames = false;
        } break;
        // don't display line numbers
        case 'n': {
          p->show_line_numbers = false;
        } break;
        // only count occurances of templates
        case 'c': {
          p->only_count = true;
        } break;
        }
      }
    } else { // handle argument without - at start
      if (p->input.count) {
        barep_strings_push(&p->p, barep_owned(arg));
      } else {
        barep_strings_push(&p->input, barep_owned(arg));
      }
    }
  }

  if (p->input.count == 0) {
    barep_error("no input are provided\n");
    return 1;
  }

  if (p->p.count == 0) {
    barep_error("no patterns are provided\n");
    return 1;
  }

  return 0;
}

// process file

int barep_process_file(const char *filename, Barep_State *s) {
  FILE *f = fopen(filename, "r");

  if (f == NULL) {
    barep_error("can't open file: %s\n", filename);
    perror("ERROR");
    return 1;
  }

  char buf[4096];
  size_t line = 0;

  while (fgets(buf, sizeof buf, f) != NULL) {
    ++line;
    Aho_Node_Ptr cur = s->dict;
    for (size_t i = 0; buf[i]; i++) {
      if ((size_t)buf[i] >= AHO_ALPHABET) { // clear state
        cur = s->dict;
        continue;
      }
      cur = aho_go(cur, buf[i]);
      s->count += aho_matches_count(cur);

      if (s->params.only_count) {
        continue;
      }

      // display occurance
      aho_for_match(x, cur) {
        int sz = aho_size(x);
        int beg = i - sz + 1;
        if (s->params.show_filenames) {
          printf("%s:", filename);
        }
        if (s->params.show_line_numbers) {
          printf("%zu:%d-%zu: ", line, beg + 1, i + 1);
        }
        printf("%s", buf);
      }
    }
  }

  fclose(f);

  return 0;
}

// help

// barep PATH [OPTIONS] [[-e] PATTERNS ...]
void barep_usage(const char *prog) { //
  printf("Usage: %s FILE [OPTIONS] [[-e] PATTERNS...]\n", prog);
  printf("barep (Bar Rep, br) searches the current directory or file\n");
  printf("for occurances of given patterns.\n");
  printf("\n");
  printf("You can provide files/directories using -f flag or first argument\n");
  printf("that isn't started at - will be checked as file/dir\n");
  printf("\n");
  printf("Use -h flag for more details.\n");
}

void barep_help() {
  printf("The following OPTIONS are accepted:\n");
  printf("-h\tPrint this usage information message and exit\n");
  printf("-c\tdon't show occurances (only count them)\n");
  printf("-e\tAdd a pattern literally (even if started with dash -)\n");
  printf("-e\tadd an input file");
  printf("-n\tDon't display line numbers\n");
  printf("-q\tdon't show filenames when display n pattern occurance\n");
  printf("\n");
  printf("Author is @semenInRussia [GitHub]\n");
}

int main(int argc, const char **argv) {
  Barep_Params p = barep_params_default();
  int r = barep_parse_args(argc, argv, &p);
  if (r != 0) {
    printf("\n");
    barep_usage(argv[0]);
    return r;
  }

  if (p.help) {
    barep_usage(argv[0]);
    barep_help();
    return 0;
  }

  // build
  Aho_Node_Ptr t = aho_make();
  for (size_t i = 0; i < p.p.count; i++) {
    aho_add(t, p.p.items[i], i);
  }
  aho_build(t);

  // process
  Barep_State s = {.dict = t, .params = p, .count = 0};
  for (size_t i = 0; i < p.input.count; i++) {
    // if file haven't matches don't display divider
    size_t old = s.count;
    barep_process_file(p.input.items[i], &s);
    if (!p.only_count && s.count > old) {
      printf("%s", BAREP_FILE_DIVIDER);
    }
  }

  // footer
  printf("Found %zu occurances of given templates", s.count);
}
