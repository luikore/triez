require "test/unit"
require_relative "../lib/triez"

GC.stress

class TriezTest < Test::Unit::TestCase
  def test_hat_trie
    t = Triez.hat
    assert_equal Triez::HatTrie, t.class

    t['万塘路一锅鸡'] = (1 << 40)
    t['万塘路'] = (1 << 41)
    assert_equal (1 << 40), t['万塘路一锅鸡']
    assert_equal (1 << 41), t['万塘路']
    assert_equal nil, t['万']
    assert_equal false, t.has_key?('万')
    assert_equal true, t.has_key?('万塘路')
  end
end
