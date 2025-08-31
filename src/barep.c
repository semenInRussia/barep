#include <assert.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

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
  char *items;
  size_t capacity;
  size_t count;
} Barep_Str;

typedef struct {
  Aho_Node_Ptr dict;
  Barep_Params params;
  size_t count; // keep amount of processed occurances
  Barep_Str pref;
} Barep_State;

// log functions

#define barep_error(fmt, ...) fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__)
#define barep_perror() perror("ERROR")

// ...

char *barep_owned(const char *s) {
  size_t len = strlen(s);
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
  while (p->count + 1 >= p->capacity) {
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
  // p.input = ; // input files/directories
  // p.p = ; // patterns

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

// strings

Barep_Str barep_str_slice(char *beg, char *end) {
  Barep_Str str;
  str.items = beg;
  str.capacity = 0;
  str.count = end - beg;
  return str;
}

Barep_Str barep_str_new() { return barep_str_slice(0, 0); }

void barep_str_free(Barep_Str *s) { free((void *)s->items); }

void barep_str_fit(Barep_Str *s, size_t sz) {
  while (s->capacity < sz) {
    s->capacity = s->capacity == 0 ? 16 : s->capacity * 2;
  }
  s->items = realloc((void *)s->items, sizeof(s->items[0]) * s->capacity);
}

void barep_str_push(Barep_Str *a, Barep_Str b) {
  barep_str_fit(a, a->count + b.count);
  memcpy((void *)&a->items[a->count], b.items, sizeof(b.items[0]) * b.count);
}

void barep_str_print(Barep_Str a) { printf("%.*s", (int)a.count, a.items); }

// process

void barep__file_print_matches(const char *filename, size_t j, size_t cnt,
                               size_t line, size_t begl, char *buf,
                               Barep_Str pref, Barep_State *s) {
  if (s->params.only_count) {
    return;
  }
  for (size_t k = 0; k < cnt; k++) {
    if (s->params.show_filenames) {
      printf("%s:", filename);
    }
    if (s->params.show_line_numbers) {
      printf("%zu:", line + 1);
    }
    barep_str_print(pref);
    barep_str_print(barep_str_slice(&buf[begl], &buf[j]));
    printf("\n");
  }
}

int barep_file(const char *filename, Barep_State *s) {
  FILE *f = fopen(filename, "r");

  if (f == NULL) {
    barep_error("can't open file: %s\n", filename);
    barep_perror();
    return 1;
  }

  // read a file with chunks size=4096
  // go character by character.
  // if a match save it
  // if end of line print matches (keep `begl` and `endl`)
  // if end of a buffer, keep prefix of a line
  // also needed `line` & `col`

  const int CHUNK = 4096;
  char buf[CHUNK];
  size_t begl = 0;
  size_t line = 0;
  size_t col = 0;
  size_t cnt = 0; // count matchces

  Aho_Node_Ptr cur = s->dict;
  size_t j;

  while (true) {
    size_t rd = fread(buf, sizeof(buf[0]), CHUNK, f);
    if (rd < CHUNK && ferror(f)) {
      barep_error("I/O while reading file: %s\n", filename);
      barep_perror();
      return 1;
    }

    for (j = 0; j < rd; j++, col++) {
      if ((size_t)buf[j] >= AHO_ALPHABET) {
        cur = s->dict;
        continue;
      }
      cur = aho_go(cur, buf[j]);
      cnt += aho_matches_count(cur);
      s->count += aho_matches_count(cur);
      if (buf[j] == '\n') {
        barep__file_print_matches(filename, j, cnt, line, begl, buf, s->pref,
                                  s);
        ++line;
        col = 0;
        s->pref.count = 0;
        cnt = 0;
        begl = j + 1;
      }
    }

    if (feof(f)) {
      break;
    }

    // read other chunk
    barep_str_push(&s->pref, barep_str_slice(buf + begl, buf + CHUNK));
    begl = 0;
  }

  barep__file_print_matches(filename, j, cnt, line, begl, buf, s->pref, s);
  fclose(f);

  return 0;
}

int barep_dir(const char *dirname, Barep_State *s) {
  HANDLE hfind;

  char pattern[MAX_PATH];
  snprintf(pattern, sizeof(pattern), "%s\\*", dirname);

  WIN32_FIND_DATA f;
  hfind = FindFirstFile(pattern, &f);

  if (hfind == INVALID_HANDLE_VALUE) {
    barep_error("can't read directory %s\n", dirname);
    return 1;
  }

  do {
    const char *fileName = f.cFileName;
    if (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0) {
      continue;
    }
    char filePath[MAX_PATH];
    snprintf(filePath, sizeof(filePath), "%s\\%s", dirname, fileName);
    if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      barep_dir(filePath, s);
    } else {
      barep_file(filePath, s);
    }
  } while (FindNextFile(hfind, &f) != 0);

  FindClose(hfind);

  return 0;
}

bool barep_is_file(const char *filename) {
  DWORD fileAttr = GetFileAttributesA(filename);
  return (fileAttr != INVALID_FILE_ATTRIBUTES &&
          !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

void barep_file_or_dir(const char *filename, Barep_State *s) {
  if (barep_is_file(filename)) {
    barep_file(filename, s);
  } else {
    barep_dir(filename, s);
  }
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
    const char *pat = p.p.items[i];
    aho_add(t, pat, i);
  }
  aho_build(t);

  // process
  Barep_State s = {.dict = t, .params = p, .count = 0};
  for (size_t i = 0; i < p.input.count; i++) {
    // if file haven't matches don't display divider
    size_t old = s.count;
    barep_file_or_dir(p.input.items[i], &s);
    if (!p.only_count && s.count > old) {
      printf("%s", BAREP_FILE_DIVIDER);
    }
  }

  // footer
  printf("Found %zu occurances of given templates", s.count);
}
