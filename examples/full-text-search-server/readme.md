A simple prove-of-concept full text search server with *triez* and *sqlite*

required gems:

``` ruby
gem ins triez sqlite3 rack thin
```

start server:

``` bash
ruby server.rb
```

client usage:

```ruby
require './client'
c = Client.new 'localhost', 9292
c.insert id, document_content
c.query a_phrase
```

An experiment of indexing 30M text of 130 documents took about 1 minute (mostly sqlite time) and 190M memory in index. Once the index is finished, the query is super fast. Loading index from database is also quite fast.
