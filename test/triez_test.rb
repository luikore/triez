require "test/unit"
require_relative "../lib/triez"

GC.stress

class TriezTest < Test::Unit::TestCase
  def test_valued_hat_trie
    t = Triez.valued_hat
    assert_equal Triez::ValuedHatTrie, t.class

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

    a = []
    t._internal_search '', nil, -> k, v { a << [k, v] }
    assert_equal [['万塘路', v2]], a

    a = []
    t['马当路'] = 3
    t._internal_search '万塘', nil, -> k, v { a << [k, v] }
    assert_equal [['路', v2]], a
  end

  def test_iter
    t = Triez.hat
    ('A'..'z').each do |a|
      # 10k chars to ensure burst
      ('一'..'百').each do |b|
        t[a + b] = 0
      end
    end

    a = []
    t._internal_search 'a', nil, -> k, v { a << k }
    assert_equal ('一'..'百').to_a, a.sort
  end
end
