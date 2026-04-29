#include <assert.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "aho.h"

// defines

#define BAREP_FILE_DIVIDER "\n"

// structs

typedef struct {
  size_t capacity;
  size_t count;
  const char **items;
} Barep_Strings;

typedef struct {
  bool show_line_numbers;
  bool show_filenames;

  Barep_Strings input;
  Barep_Strings p;

  bool only_count;
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
  size_t count;
  Barep_Str pref;
} Barep_State;

// log functions

#define barep_error(fmt, ...) fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__)
#define barep_perror() perror("ERROR")

// utils

char *barep_owned(const char *s) {
  size_t len = strlen(s);

  char *out = (char *)malloc(len + 1);
  if (out == NULL) {
    barep_perror();
    exit(1);
  }

  memcpy(out, s, len + 1);
  return out;
}

// vector of strings

void barep_strings_reserve(Barep_Strings *p, size_t n) {
  if (n > p->capacity) {
    const char **new_items =
        (const char **)realloc(p->items, n * sizeof(const char *));
    if (new_items == NULL) {
      barep_perror();
      exit(1);
    }

    p->items = new_items;
    p->capacity = n;
  }
}

void barep_strings_push(Barep_Strings *p, const char *s) {
  if (p->capacity == 0) {
    barep_strings_reserve(p, 2);
  }

  if (p->count == p->capacity) {
    barep_strings_reserve(p, p->capacity * 2);
  }

  p->items[p->count++] = s;
}

void barep_strings_free(Barep_Strings p) {
  for (size_t i = 0; i < p.count; ++i) {
    free((void *)p.items[i]);
  }

  free(p.items);
}

// params

Barep_Params barep_params_default(void) {
  Barep_Params p = {0};

  p.show_line_numbers = true;
  p.show_filenames = true;

  p.only_count = false;
  p.help = false;

  return p;
}

void barep_params_free(Barep_Params p) {
  barep_strings_free(p.input);
  barep_strings_free(p.p);
}

#define barep_shift(argc, argv) (assert((argc) > 0), --(argc), ++(argv))

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
        case 'h':
          p->help = true;
          return 0;

        case 'e':
          if (nxt) {
            barep_error("two flags requiring arguments cannot be in the same "
                        "argument\n");
            return 1;
          }

          if (argc == 0) {
            barep_error("-e requires pattern\n");
            return 1;
          }

          nxt = true;
          barep_strings_push(&p->p, barep_owned(argv[0]));
          barep_shift(argc, argv);
          break;

        case 'f':
          if (nxt) {
            barep_error("two flags requiring arguments cannot be in the same "
                        "argument\n");
            return 1;
          }

          if (argc == 0) {
            barep_error("-f requires filename\n");
            return 1;
          }

          nxt = true;
          barep_strings_push(&p->input, barep_owned(argv[0]));
          barep_shift(argc, argv);
          break;

        case 'q':
          p->show_filenames = false;
          break;

        case 'n':
          p->show_line_numbers = false;
          break;

        case 'c':
          p->only_count = true;
          break;

        default:
          barep_error("unknown option: -%c\n", arg[i]);
          return 1;
        }
      }
    } else {
      if (p->input.count) {
        barep_strings_push(&p->p, barep_owned(arg));
      } else {
        barep_strings_push(&p->input, barep_owned(arg));
      }
    }
  }

  if (p->input.count == 0) {
    barep_error("no input provided\n");
    return 1;
  }

  if (p->p.count == 0) {
    barep_error("no patterns provided\n");
    return 1;
  }

  return 0;
}

// strings

Barep_Str barep_str_slice(char *beg, char *end) {
  Barep_Str str;

  str.items = beg;
  str.capacity = 0;
  str.count = (size_t)(end - beg);

  return str;
}

Barep_Str barep_str_new(void) { return barep_str_slice(NULL, NULL); }

void barep_str_free(Barep_Str *s) {
  free(s->items);

  s->items = NULL;
  s->capacity = 0;
  s->count = 0;
}

void barep_str_fit(Barep_Str *s, size_t sz) {
  if (s->capacity >= sz) {
    return;
  }

  while (s->capacity < sz) {
    s->capacity = s->capacity == 0 ? 16 : s->capacity * 2;
  }

  char *new_items =
      (char *)realloc(s->items, sizeof(s->items[0]) * s->capacity);
  if (new_items == NULL) {
    barep_perror();
    exit(1);
  }

  s->items = new_items;
}

void barep_str_push(Barep_Str *a, Barep_Str b) {
  barep_str_fit(a, a->count + b.count);

  memcpy(&a->items[a->count], b.items, sizeof(b.items[0]) * b.count);

  a->count += b.count;
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

  enum { CHUNK = 4096 };

  char buf[CHUNK];

  size_t begl = 0;
  size_t line = 0;
  size_t cnt = 0;

  Aho_Node_Ptr cur = s->dict;
  size_t j = 0;

  while (true) {
    size_t rd = fread(buf, sizeof(buf[0]), CHUNK, f);

    if (rd < CHUNK && ferror(f)) {
      barep_error("I/O error while reading file: %s\n", filename);
      barep_perror();
      fclose(f);
      return 1;
    }

    for (j = 0; j < rd; j++) {
      unsigned char ch = (unsigned char)buf[j];

      if ((size_t)ch >= AHO_ALPHABET) {
        cur = s->dict;
      } else {
        cur = aho_go(cur, ch);

        size_t matches = aho_matches_count(cur);
        cnt += matches;
        s->count += matches;
      }

      if (buf[j] == '\n') {
        barep__file_print_matches(filename, j, cnt, line, begl, buf, s->pref,
                                  s);

        ++line;
        s->pref.count = 0;
        cnt = 0;
        begl = j + 1;
      }
    }

    if (feof(f)) {
      break;
    }

    barep_str_push(&s->pref, barep_str_slice(buf + begl, buf + rd));
    begl = 0;
  }

  barep__file_print_matches(filename, j, cnt, line, begl, buf, s->pref, s);

  fclose(f);
  return 0;
}

// Linux filesystem

bool barep_is_file(const char *path) {
  struct stat st;

  if (stat(path, &st) != 0) {
    return false;
  }

  return S_ISREG(st.st_mode);
}

bool barep_is_dir(const char *path) {
  struct stat st;

  if (stat(path, &st) != 0) {
    return false;
  }

  return S_ISDIR(st.st_mode);
}

int barep_dir(const char *dirname, Barep_State *s) {
  DIR *dir = opendir(dirname);

  if (dir == NULL) {
    barep_error("can't read directory: %s\n", dirname);
    barep_perror();
    return 1;
  }

  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    const char *name = entry->d_name;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    char path[PATH_MAX];

    int n = snprintf(path, sizeof(path), "%s/%s", dirname, name);
    if (n < 0 || (size_t)n >= sizeof(path)) {
      barep_error("path is too long: %s/%s\n", dirname, name);
      continue;
    }

    struct stat st;

    if (stat(path, &st) != 0) {
      barep_error("can't stat path: %s\n", path);
      barep_perror();
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      barep_dir(path, s);
    } else if (S_ISREG(st.st_mode)) {
      barep_file(path, s);
    }
  }

  closedir(dir);
  return 0;
}

void barep_file_or_dir(const char *path, Barep_State *s) {
  if (barep_is_file(path)) {
    barep_file(path, s);
  } else if (barep_is_dir(path)) {
    barep_dir(path, s);
  } else {
    barep_error("not a regular file or directory: %s\n", path);
  }
}

// help

void barep_usage(const char *prog) {
  printf("Usage: %s FILE [OPTIONS] [[-e] PATTERNS...]\n", prog);
  printf("barep searches the given directory or file\n");
  printf("for occurrences of given patterns.\n");
  printf("\n");
  printf("You can provide files/directories using -f flag or first argument\n");
  printf("that doesn't start with - will be checked as file/dir.\n");
  printf("\n");
  printf("Use -h flag for more details.\n");
}

void barep_help(void) {
  printf("The following OPTIONS are accepted:\n");
  printf("-h\tPrint usage information and exit\n");
  printf("-c\tDon't show occurrences, only count them\n");
  printf("-e\tAdd a pattern literally, even if it starts with '-'\n");
  printf("-f\tAdd an input file or directory\n");
  printf("-n\tDon't display line numbers\n");
  printf("-q\tDon't show filenames\n");
  printf("\n");
  printf("Author is @semenInRussia [GitHub]\n");
}

int main(int argc, const char **argv) {
  Barep_Params p = barep_params_default();

  int r = barep_parse_args(argc, argv, &p);
  if (r != 0) {
    printf("\n");
    barep_usage(argv[0]);
    barep_params_free(p);
    return r;
  }

  if (p.help) {
    barep_usage(argv[0]);
    barep_help();
    barep_params_free(p);
    return 0;
  }

  Aho_Node_Ptr t = aho_make();

  for (size_t i = 0; i < p.p.count; i++) {
    const char *pat = p.p.items[i];
    aho_add(t, pat, i);
  }

  aho_build(t);

  Barep_State s = {
      .dict = t,
      .params = p,
      .count = 0,
      .pref = {0},
  };

  for (size_t i = 0; i < p.input.count; i++) {
    size_t old = s.count;

    barep_file_or_dir(p.input.items[i], &s);

    if (!p.only_count && s.count > old) {
      printf("%s", BAREP_FILE_DIVIDER);
    }
  }

  printf("Found %zu occurrences of given templates\n", s.count);

  barep_str_free(&s.pref);
  barep_params_free(p);

  // Если в aho.h есть функция освобождения дерева, вызови её здесь:
  // aho_free(t);

  return 0;
}
