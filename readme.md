## What

Pragmatic trie for Ruby, fast, memory efficient, unicode aware.

The backend is a cache oblivious data structure: the HAT trie.

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

t = Triez.new

# insertion
t[word] = an_int_within_64_bits

# insert default value (0 for normal triez, nil for obj_valued triez) for key
t << word

# search
t.has_key? word
t[word]

# enumerate (NOTE it is unordered)
t.each do |key, value|
  ...
end

# iterate over values under a prefix.
t.search_with_prefix(prefix, limit: 10, sort: true) do |suffix, value|
  ...
end
```

---

A triez stores integers within 64bits by default, good enough for weights, counts or database IDs, and doesn't cost any time in GC marking phase. In case you need to store arbitrary object value in a node:

``` ruby
t = Triez.new obj_value: true
t['Tom'] = {name: 'Tom', sex: 'Female'}
t['Tree'] = [:leaf, :trunk, :root]
```

---

A suffix trie inserts all suffices of a key

``` ruby
t = Triez.new suffix: true
t['abcd'] = 2
t['d']    #=> 2
t['cd']   #=> 2
t['bcd']  #=> 2
t['abcd'] #=> 2
```

You can mutate values with a block

``` ruby
# v *= 5 for 'abcd', 'bcd', 'cd', 'd'
t.alt 'abcd' do |v|
  v * 5
end
t['abcd'] #=> 10
t['cd']   #=> 10
```

---

Misc methods

``` ruby
# if it is a suffix trie
t.suffix?
# if the value type is object
t.obj_value?
```

## Examples

Prefix-based autocompletion:

``` ruby
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

---

Efficiently search for strings containing a substring:

``` ruby
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

Search time is linear to the length of the substring.

## Benchmarks



## Caveats

- `sort` orders keys with binary collation, not unicode codepoint collation in string comparison.
- For some rare case of many threads modifying the same trie, you may need a mutex.
