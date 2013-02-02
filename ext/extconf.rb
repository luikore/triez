require "mkmf"
create_makefile 'tries'

def sh *xs
  puts xs.join(' ')
  system *xs
end

require "fileutils"
include FileUtils
cd File.dirname(__FILE__) + '/build'
unless File.exist?('libtries.a')
  cc = RbConfig::CONFIG['CC']
  ar = RbConfig::CONFIG['AR']
  sh cc, '-c', '-I..', *Dir.glob("../hat-trie/*.c")
  sh cc, '-c', '-Imarisa-trie', *Dir.glob("../marisa-trie/**/*.cc")
  sh ar, '-r', 'libtries.a', *Dir.glob("*.o")
end
