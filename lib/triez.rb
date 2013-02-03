require_relative "../ext/triez"

class Triez
  VERSION = '0.1'

  def initialize opts={}
    opts = opts.dup
    obj_value = opts.delete :obj_value
    obj_value = false if obj_value.nil?
    suffix    = opts.delete :suffix
    suffix    = false if suffix.nil?
    unless opts.empty?
      raise ArgumentError, "Unknown options: #{opts.keys.inspect}, only [:suffix, :obj_value] are allowed"
    end
    _internal_set_type obj_value, suffix
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
      self
    else
      a = []
      _internal_search prefix, limit, sort, -> k, v {a << [k, v]}
      a
    end
  end
end
