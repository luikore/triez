require "test/unit"
require_relative "../lib/triez"

GC.stress

class TriezTest < Test::Unit::TestCase
  def test_hat_trie
    t = Triez.new obj_value: true

    v1 = (1 << 40)
    v2 = (1 << 141)
    t['万塘路一锅鸡'] = v1
    t['万塘路'] = v2
    assert_equal v1, t['万塘路一锅鸡']
    assert_equal v2, t['万塘路']
    assert_equal nil, t['万']
    assert_equal false, t.has_key?('万')
    assert_equal true, t.has_key?('万塘路')

    assert_equal v1, t.delete('万塘路一锅鸡')
    assert_equal nil, t['万塘路一锅鸡']
    assert_equal v2, t['万塘路']

    a = t.search_with_prefix ''
    assert_equal [['万塘路', v2]], a

    t['马当路'] = 3
    a = t.search_with_prefix '万塘'
    assert_equal [['路', v2]], a
  end

  def test_insertion_and_search_on_many_keys
    t = Triez.new
    as = ('A'..'z').to_a
    bs = ('一'..'百').to_a
    as.each do |a|
      # 10k chars to ensure burst
      bs.each do |b|
        t[a + b] = 0
      end
    end
    assert_equal as.size * bs.size, t.size

    a = t.search_with_prefix 'a'
    assert_equal bs.to_a, a.map(&:first).sort

    a = []
    t.search_with_prefix 'b', sort: true, limit: 3 do |k, v|
      a << k
    end
    assert_equal 3, a.size
    assert_equal a, a.sort
  end

  def test_each_and_raise
    t = Triez.new
    t['abcd'] = 0
    t['abc'] = 1

    assert_raise NameError do
      t.each do |k, v|
        raise NameError, k
      end
    end

    assert_raise ArgumentError do
      t.each
    end
  end
end
