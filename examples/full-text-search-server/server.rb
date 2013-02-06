# coding: utf-8

ENV['RACK_ENV'] = 'production'

require "triez"
require 'sqlite3'
require "rack"
require "thin"
require "fileutils"
FileUtils.cd File.dirname(__FILE__)

INDEX = Triez.new
DB = SQLite3::Database.new 'ids.db'

class Server
  def initialize
    if DB.table_info('phrases').empty?
      init_table
    else
      load_index
    end
  end

  def load_index
    count, _ = DB.execute('select count(*) from phrases;').first
    puts "loading #{count} phrases from db"
    last_id = 0
    (count / 30000 + 1).times do |i|
      DB.execute('select id, phrase from phrases where id > ? limit 30000;', last_id).each do |(id, phrase)|
        INDEX.change_all(:suffix, phrase) { id }
        last_id = id
      end
    end
  end

  def init_table
    puts 'initializeing table'
    DB.execute <<-SQL
      create table phrases (
        id integer primary key asc,
        phrase string,   -- a string segment in document
        doc_ids string   -- represent documents containing the phrase, in the form of {id => 0}
      );
    SQL
  end

  def insert_phrase doc_id, phrase
    id = INDEX[phrase]
    if id > 0
      # todo explode if too big
      doc_ids = Marshal.load (DB.execute 'select doc_ids from phrases where id=?;', id).first.first
      doc_ids[doc_id] = 0
      DB.execute 'update phrases set doc_ids=? where id=?;', Marshal.dump(doc_ids), id
    else
      DB.execute 'insert into phrases (phrase,doc_ids) values (?,?);', phrase, Marshal.dump(doc_id => 0)
      id = DB.last_insert_row_id
      INDEX.change_all(:suffix, phrase) { id }
    end
  end

  def insert input
    doc_id, phrases = input
    DB.transaction do
      phrases.each do |ph|
        insert_phrase doc_id, ph
      end
    end
    puts "#{phrases.size} phrases inserted for #{doc_id}"
  end

  def query phrase
    ids = INDEX.search_with_prefix(phrase, limit: 10).map &:last
    if ids.empty?
      ids
    else
      # no injection if we don't expose it!
      rs = DB.execute "select doc_ids from phrases where id in (#{ids.sort.join ','});"
      h = {}
      rs.each do |(r)|
        h.merge! Marshal.load r
      end
      h.keys
    end
  end

  def call env
    input = env['rack.input'].read
    case env['REQUEST_PATH']
    when '/a'
      insert Marshal.load input
      [200, {"Content-Length" => '0'}, ['']]
    when '/q'
      r = Marshal.dump query input
      [200, {"Content-Length" => r.bytesize.to_s}, [r]]
    end
  rescue
    r = $!.message
    [500, {"Content-Length" => r.bytesize}, [r]]
  end
end

Rack::Handler::Thin.run Server.new, Port: 9292
