require_relative "../ext/triez"

class Triez
  VERSION = '1.0.4'

  private :_internal_set_type
  private :_internal_search
  private :_internal_walk

  def initialize opts={}
    opts = opts.dup

    value_type = opts.delete :value_type
    if value_type.nil?
      value_type = :int64
    elsif value_type != :int64 and value_type != :object
      raise ArgumentError, "value_type should be :int64 or :object, but got #{value_type.inspect}"
    end

    default = opts.delete :default
    if default.nil?
      default = (value_type == :int64 ? 0 : nil)
    elsif value_type == :int64
      default = default.to_i
    end

    unless opts.empty?
      raise ArgumentError, "Unknown options: #{opts.keys.inspect}, only [:value_type, :default] are allowed"
    end

    _internal_set_type value_type == :object, default
  end

  def each &p
    raise ArgumentError, 'Need a block' unless p

    _internal_search '', nil, true, p
  end

  def walk s, &p
    _internal_walk(s).each &p
  end

  def search_with_prefix prefix, opts={}, &p
    opts  = opts.dup

    limit = opts.delete :limit
    if !limit.nil? and limit < 0
      raise ArgumentError, "Limit should be > 0"
    end

    sort = opts.delete :sort
    unless opts.empty?
      raise ArgumentError, "Unknown options: #{opts.keys.inspect}, only [:limit, :sort] are allowed"
    end

    if p
      _internal_search prefix, limit, sort, p
    else
      a = []
      _internal_search prefix, limit, sort, -> k, v {a << [k, v]}
      a
    end
  end
end
