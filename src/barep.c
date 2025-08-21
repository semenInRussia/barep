#include "aho.h"

#include <windows.h>

// return -1, if error happened, otherwise count occurances of words from given
// aho corasick tree (must be already built) in a given file
int aho_count_in_file(Aho_Node_Ptr t, FILE *f) {
  int ans = 0;
  Aho_Node_Ptr cur = t;
  char c;
  while ((c = fgetc(f)) != EOF) {
    if ((size_t)c >= AHO_ALPHABET) {
      cur = t; // clear
      continue;
    }
    cur = aho_go(cur, c);
    ans += aho_count(cur);
  }

  if (ferror(f)) {
    return -1;
  }

  return ans;
}

// return -1, if error happened, otherwise count occurances of words from given
// aho corasick tree (must be already built) in files of a given directory
int aho_count_in_dir(Aho_Node_Ptr t, const char *dirname) {
  HANDLE hfind;

  char pattern[MAX_PATH];
  snprintf(pattern, sizeof(pattern), "%s\\*", dirname);

  WIN32_FIND_DATA f;
  hfind = FindFirstFile(pattern, &f);
  if (hfind == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "ERROR: can't read directory %s\n", pattern);
    return -1;
  }

  int ans = 0;

  do {
    const char *filename = f.cFileName;
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
      continue;
    }
    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "%s\\%s", dirname, filename);
    int res = -1;
    if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      res = aho_count_in_dir(t, file_path);
    } else {
      FILE *f = fopen(file_path, "r");
      if (f == NULL) {
        fprintf(stderr, "ERROR: can't read file %s\n", file_path);
        perror("ERROR: ");
        continue;
      }
      res = aho_count_in_file(t, f);
      fclose(f);
      if (res == -1) {
        fprintf(stderr, "ERROR: I/O error on file %s\n", file_path);
      }
    }
    ans += res;
  } while (FindNextFile(hfind, &f) != 0);

  FindClose(hfind);

  return ans;
}

void usage(const char *program) {
  printf("Usage: %s <input> ... [templates]\n", program);
}

int main(int argc, const char *argv[]) {
  // argv[1] - filename
  // argv[2], argv[3], ... - templates
  //
  // display amount of occurances of templates inside text

  if (argc == 1) {
    usage(argv[0]);
    fprintf(stderr, "ERROR: no input is provided\n");
    return 1;
  }

  Aho_Node_Ptr t = aho_make();
  for (int i = 2; i < argc; i++) {
    aho_add(t, argv[i]);
  }
  aho_build(t);

  const char *filename = argv[1];

  int ans = aho_count_in_dir(t, filename);
  if (ans == -1) {
    fprintf(stderr, "ERROR: I/O error when reading\n");
    return 1;
  }

  printf("In directory %s\n", filename);
  printf("found %d occurances of given %d templates\n", ans, argc - 2);
}
