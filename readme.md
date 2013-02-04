## What

Pragmatic [tries](http://en.wikipedia.org/wiki/Trie) for Ruby, spelled in lolcat.

It is fast, memory efficient, unicode aware, prefix searchable.

The backend of *triez* is a cache oblivious data structure: the [HAT trie](https://github.com/dcjones/hat-trie). It is generally faster and more memory efficient than double arrays or burst tries.

## Requirement

- Ruby 1.9
- `g++` or `clang`

## Install

``` bash
gem ins triez
```

## Synopsis

``` ruby
require 'triez'

# create triez
t = Triez.new
t = Triez.new suffix: true
t = Triez.new obj_value: true

# if it is a suffix trie
t.suffix?

# if the value type is object
t.obj_value?

# insert or change value
t['key'] = 100

# change value with proc
t.alt 'key' do ||
end

# insert a key with default value (0 for normal triez, nil for obj_valued triez)
t << 'key'

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
```

By default, *triez* store signed integers within 64bits, you can use them as weights, counts or database IDs. It won't cost any time in GC marking phase. In case you need to store arbitrary object in a node, use `obj_value: true`:

``` ruby
t = Triez.new obj_value: true
t['Tom'] = {name: 'Tom', sex: 'Female'}
t['Tree'] = [:leaf, :trunk, :root]
```

When a *triez* is initialized with `suffix: true`, it inserts all suffices of a key

``` ruby
t = Triez.new suffix: true
t['万塘路一锅鸡'] = 2
t['万塘路一锅鸡'] #=> 2
t['塘路一锅鸡'] #=> 2
t['路一锅鸡']  #=> 2
t['一锅鸡']   #=> 2
t['锅鸡']    #=> 2
t['鸡']     #=> 2
```

You can batch change values with `alt` and a block

``` ruby
# v *= 5 for 'abcd', 'bcd', 'cd', 'd'
t.alt 'abcd' do |v|
  v * 5
end
t['abcd'] #=> 10
t['cd']   #=> 10
```

## Examples

Prefix-based autocompletion:

``` ruby
require 'triez'
words = %w[readme, rot, red, rah, rasterization]
t = Triez.new
words.each do |word|
  t[word] = 1
end
t.search_with_prefix 're' do |word|
  puts "candidate: #{word}"
end
```

The output:

```bash
candidate: readme
candidate: red
```

Efficiently search for strings containing a substring:

``` ruby
require 'triez'
sequences = {
  'ACTGAAAAAAACTG' => 1,
  'ATACGGTCCA' => 2,
  'GCTTGTACGT' => 3
}
t = Triez.new suffix: true
sequences.each do |seq, id|
  t[seq] = id
end
t.search_with_prefix 'CGGT' do |_, id|
  puts id #=> 2
end
```

The search time is linear to the length of the substring.

## Benchmarks

Here's a benchmark on

```ruby
ruby 1.9.3p374 (2013-01-15 revision 38858) [x86_64-darwin12.2.1]
2.3 GHz Intel Core i7
```

The test data is 3 milion titles of wikipedia articles (from http://dumps.wikimedia.org/enwiki/20121101/)

```
thing/backend      | memory  | insertion time | 3 M query
-------------------|---------|----------------|----------
hash/linked hash   | 340.2 M |    4.369 s     | 0.2800 s
trie/double array* | 155.6 M |    130.7 s     | 0.4359 s
triez/HAT trie     | 121.7 M |    3.872 s     | 0.3472 s
```

NOTE: `trie/double array` -> https://github.com/tyler/trie

## Caveats

- `sort` orders keys with binary collation, not unicode codepoint collation in string comparison.
- For some rare case of many threads modifying the same trie, you may need a mutex.
- If you still feel memory not enough, you may consider [MARISA-trie](https://code.google.com/p/marisa-trie/) (note that MARISA is immutable) or a database.
- It is not very efficient to use suffix trie to find longest common substring.

## Development

``` bash
git clone git://github.com/luikore/triez.git
cd triez
rake glob_src
rake
```
