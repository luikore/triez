# coding: utf-8

require "net/http"

class Client
  include Net

  def initialize addr, port
    @addr, @port = addr, port
  end

  def insert doc_id, doc
    # we keep inner spaces for precise search
    phrases = doc.split(/[\p{Punct}\p{Symbol}\r\n]/).select{|s|
      s.strip!
      s.size > 1
    }
    request '/a', Marshal.dump([doc_id, phrases])
  end

  def query phrase
    # still post request, a bit weird, but simple
    Marshal.load (request '/q', phrase)
  end

  def request path, body
    HTTP.start @addr, @port do |http|
      # TODO process 500
      http.post path, body do |str|
        return str
      end
    end
  end
end
