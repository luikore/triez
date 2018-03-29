## Triez

[![Build Status](https://travis-ci.org/luikore/triez.png)](https://travis-ci.org/luikore/triez)
[![Code Climate](https://codeclimate.com/github/luikore/triez.png)](https://codeclimate.com/github/luikore/triez)
[![Gem Version](https://badge.fury.io/rb/triez.png)](http://badge.fury.io/rb/triez)

Pragmatic [tries](http://en.wikipedia.org/wiki/Trie) for Ruby, spelled in lolcat.

It is fast, memory efficient, unicode aware, prefix searchable, and enchanced with prefix/suffix/substring keys.

The backend of *triez* is a cache oblivious data structure: the [HAT trie](https://github.com/dcjones/hat-trie) (In fact it is a [modified version](https://github.com/luikore/hat-trie) for improved functionality). HAT trie is generally faster and more memory efficient than [double array](http://linux.thai.net/~thep/datrie/datrie.html) or [burst trie](http://ww2.cs.mu.oz.au/~jz/fulltext/acmtois02.pdf).

## Requirement

- CRuby 1.9 / 2.0
- `g++` or `clang`

## Install

``` bash
gem ins triez
```

## Synopsis

``` ruby
require 'triez'

# create triez with (default value type) int64, and setting default value 0
t = Triez.new

# The default value is 0
t['foo'] #=> 0

# available options for value_type:
# - :int64  -- signed 64 bit integer
# - :object -- ruby object, but note that:
#              if the object is beyond simple types like NilClass, TrueClass, Integer,
#              it will take additional heap space
t = Triez.new value_type: :int64

# more flexible with object type [*see note below]
t = Triez.new value_type: :object

# get the value type
t.value_type

# set a different default value
t = Triez.new value_type: :object, default: 'hello'

# insert or change value
t['key'] = 100

# insert a key with default value
t << 'key'

# batch change values under all suffices/prefices/substrings of a key
t.change_all(:suffix, 'key') {|old_value| ...calculate new value }
t.change_all(:prefix, 'key') {|old_value| ...calculate new value }
# enumerates all occurences of substrings of the key
t.change_all(:substring, 'key') {|old_value| ...calculate new value }

# size of inserted keys
t.size

# search with exact match
t.has_key? 'key'
t['key']

# prefixed search (iterate over values under a prefix), available options are:
# - limit: max items, `nil` means no limit
# - sort: whether iterate in alphabetic order, default is true
t.search_with_prefix(prefix, limit: 10, sort: true) do |suffix, value|
  ...
end

# if no block given, an array in the form of [[suffix, value]] is returned
t.search_with_prefix('prefix')

# enumerate all keys and values in the order of binary collation
t.each do |key, value|
  ...
end

# iterate stored keys which are prefices of a given string, from shallow to deep
t.walk string do |k, v|
  ...
end
```

\* Note: By default, *triez* store signed integers within 64bits, you can use them as weights, counts or database IDs. In case you need to store arbitrary object in a node, use `value_type: :object`:

``` ruby
t = Triez.new value_type: :object
t['Tom'] = {name: 'Tom', sex: 'Female'}
t['Tree'] = [:leaf, :trunk, :root]
```

## Examples

**Prefix based autocompletion**:

``` ruby
require 'triez'
words = %w[readme, rot, red, rah, rasterization]
t = Triez.new
words.each do |word|
  t[word] = 1
end
t.search_with_prefix 're' do |suffix|
  puts "candidate: re#{suffix}"
end
```

The output:

```bash
candidate: readme
candidate: red
```

---

**Efficient [full text search](https://en.wikipedia.org/wiki/Full_text_search) with a [suffix tree](https://en.wikipedia.org/wiki/Suffix_tree)**:

``` ruby
require 'triez'
sequences = {
  'ACTGAAAAAAACTG' => 1,
  'ATACGGTCCA' => 2,
  'GCTTGTACGT' => 3
}
t = Triez.new

# build suffix tree
sequences.each do |seq, id|
  t.change_all(:suffix, seq){id}
end

t.search_with_prefix 'CGGT' do |_, id|
  puts id #=> 2
end
```

The searching time is linear to the length of the substring. You may also be interested in the example of a simple [full text search server](https://github.com/luikore/triez/tree/master/examples/full-text-search-server) with *triez*.

---

**Solve the [longest common substring problem](https://en.wikipedia.org/wiki/Longest_common_substring_problem)**:

``` ruby
# coding: utf-8
require 'triez'
sentences = %w[
  万塘路一锅鸡
  去文二路一锅鸡吃饭
  来一锅鸡顶盒
  一锅鸡胗
]

# value is bitset representing id of the sentence
# in ruby we can use integers of arbitrary length as bitsets
t = Triez.new value_type: :object, default: 0

sentences.each_with_index do |sentence, i|
  elem = 1 << i
  t.change_all :substring, sentence do |v|
    # union
    v | elem
  end
end

# longest common substring
lcs = ''

# find the key tagged with universe
universe = (1 << sentences.size) - 1
t.each do |k, v|
  lcs = k if k.size > lcs.size and v == universe
end

puts lcs #=> 一锅鸡
```

## Benchmark

Here's a benchmark on

```ruby
ruby 1.9.3p374 (2013-01-15 revision 38858) [x86_64-darwin12.2.1]
2.3 GHz Intel Core i7
```

The test data are 3 milion titles of wikipedia articles (from http://dumps.wikimedia.org/enwiki/20121101/)

```
thing/backend           | memory  | insertion time | 3 M query
------------------------|---------|----------------|----------
hash/linked hash        | 340.2 M |    4.369 s     | 0.2800 s
fast_trie/double array* | 155.6 M |    130.7 s     | 0.4359 s
triez/HAT trie          | 121.7 M |    3.872 s     | 0.3472 s
```

Note: `fast_trie/double array` -> https://github.com/tyler/trie

## Caveats

- The `sort` option in prefixed search orders keys with binary [collation](https://en.wikipedia.org/wiki/Collation), but string comparison in Ruby is with unicode codepoint collation.
- For some rare case of many threads modifying the same trie, you may need a mutex.
- If you still feel memory not enough, you may consider [MARISA-trie](https://code.google.com/p/marisa-trie/) (note that MARISA is immutable), or a database.

## Development

``` bash
git clone git://github.com/luikore/triez.git
cd triez
rake glob_src
rake
```

To update vendor lib and re-compile:

``` bash
rake glob_src
rake
```

## Note

Although HAT trie uses MurMurHash3 instead of SipHash in Ruby, It is still safe under hashDoS because bucket size is limited.
