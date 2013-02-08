# coding: utf-8

require "drb/drb"

class Client
  def initialize addr, port
    @server = DRbObject.new_with_uri "druby://#{addr}:#{port}"
  end

  def insert doc_id, doc
    # we keep inner spaces for precise search
    phrases = doc.split(/[\p{Punct}\p{Symbol}\r\n]+/).select{|s|
      s.strip!
      s.size > 1
    }
    @server.insert doc_id, phrases
  end

  def query phrase
    @server.query phrase
  end
end
