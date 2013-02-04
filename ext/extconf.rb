require "mkmf"

$CFLAGS << ' -Ihat-trie'
$LDFLAGS << ' -Lbuild -ltries'
create_makefile 'triez'

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
build_dir = File.dirname(__FILE__) + '/build'
mkdir_p build_dir
cd build_dir
unless File.exist?('libtries.a')
  cc = ENV['CC'] || RbConfig::CONFIG['CC']
  cc = [cc, '-O3', '-c']
  ar = RbConfig::CONFIG['AR']
  ar = 'ar' unless File.exist?(ar)
  sh *cc, '-I..', *Dir.glob("../hat-trie/*.c")
  sh ar, '-r', 'libtries.a', *Dir.glob("*.o")
end
