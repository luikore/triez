require "mkmf"

$CFLAGS << ' -Ihat-trie -Imarisa-trie'
$LDFLAGS << ' -Lbuild -ltries'
create_makefile 'triex'

# respect header changes
headers = Dir.glob('*.{hpp,h}').join ' '
File.open 'Makefile', 'a' do |f|
  f.puts "\n$(OBJS): #{headers}"
end

# build vendor lib
def sh *xs
  puts xs.join(' ')
  system *xs
end

require "fileutils"
include FileUtils
cd File.dirname(__FILE__) + '/build'
unless File.exist?('libtries.a')
  cc = ENV['CC'] || RbConfig::CONFIG['CC']
  ar = RbConfig::CONFIG['AR']
  ar = 'ar' unless File.exist?(ar)
  sh cc, '-c', '-I..', *Dir.glob("../hat-trie/*.c")
  sh cc, '-c', '-Imarisa-trie', *Dir.glob("../marisa-trie/**/*.cc")
  sh ar, '-r', 'libtries.a', *Dir.glob("*.o")
end
