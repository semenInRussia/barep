# Bar ReP (brp)

`barep` is a search tool similar to [grep](https://www.gnu.org/software/grep/) or [ripgrep](https://github.com/BurntSushi/ripgrep), but `barep` is written in the most powerful language C by [me](https://github.com/semenInRussia/ "@semenInRussia"), so it's even better.

The killer feature of this tool is that it doesn't support regular expressions

```
Usage: ./brp FILE [OPTIONS] [[-e] PATTERNS...]
barep searches the given directory or file
for occurrences of given patterns.

You can provide files/directories using -f flag or first argument
that doesn't start with - will be checked as file/dir.

Use -h flag for more details.
The following OPTIONS are accepted:
-h	Print usage information and exit
-c	Don't show occurrences, only count them
-e	Add a pattern literally, even if it starts with '-'
-f	Add an input file or directory
-j	Use up to N worker processes (Linux/macOS)
-n	Don't display line numbers
-q	Don't show filenames

Author is @semenInRussia [GitHub]
```

## under the hood

I am use algorithm called [Aho-Corasick](https://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_algorithm), not KMP.  The reason is that in KMP you need to store a source in any buffer.  When you use Aho-Corasick you just pass characters and accept integer it's all you need to store through reading file.  Also Aho-Corasick allow to have more than one templates almost without changes in speed and memory.

Note that grep and ripgrep search engines use finite-state machines and aho-corasick is it
