# Bar Rep (br)

`barrep` is a search tool similar to [grep](https://www.gnu.org/software/grep/) or [ripgrep](https://github.com/BurntSushi/ripgrep), but `barrep` is written in the most powerful language C by [me](https://github.com/semenInRussia/ "@semenInRussia"), so it's even better.

The killer feature of this tool is that it doesn't support regular expressions

## under the hood

I am use algorithm called [Aho-Corasick](https://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_algorithm), not KMP.  The reason is that in KMP you need to store a source in any buffer.  When you use Aho-Corasick you just pass characters and accept integer it's all you need to store through reading file.  Also Aho-Corasick allow to have more than one templates almost without changes in speed and memory.

Note that grep and ripgrep search engines use finite automata and aho-corasick is it
