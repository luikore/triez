## What

This is Ruby binding for modern trie implementations, unicode aware, providing Hash-like API.

## Requirement

- Ruby 1.9
- GCC or Clang

## Install

``` bash
gem ins triez
```

## Usage

``` ruby
require 'triez'
```

---

HAT trie is mutable. It is faster than double array. It takes much less memory than just storing keys and values in a Hash.

``` ruby
hat = TrieX.hat

# insertion
words.each do |word|
  hat[word] = word.size
end

# search
hat.has_key? word
hat[word]

# enumerate (ordered by alphabet)
hat.each do |key, value|
  ...
end
```

And you can retrieve sub-tree with a prefix, suitable for prefix-based completions.

``` ruby
hat.search(prefix, limit: 10).each do |suffix, value|
  ...
end
```

---

The default HAT trie stores integers within 64bits, it is good enough for weights, database ids... and takes 0 time in GC, and copy-on-write friendly.

But there may be cases you want to store arbitrary objects in the nodes, then use this constructor instead:

``` ruby
hat_trie = TrieX.valued_hat
```

---

MARISA trie is immutable, and provides **no** sub-tree walking API, but the memory footprint is extremely small (nearly as small as zipped).

``` ruby
marisa = TrieX.marisa

# insertion
words.each do |word|
  marisa[word] = word.size
end

# search
marisa[word]
```

## License

See the copying for HAT-trie and MARISA-trie
