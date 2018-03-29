require "mkmf"

$CFLAGS << ' -Ihat-trie'
$CPPFLAGS << ' -Ihat-trie'
$LDFLAGS << ' -Lbuild -ltries'
create_makefile 'triez'

# respect header changes
headers = Dir.glob('*.{hpp,h}').join ' '
File.open 'Makefile', 'a' do |f|
  f.puts
  f.puts "$(OBJS): #{headers}"
  f.puts "$(DLLIB): build/libtries.a"
  f.puts "build/libtries.a:"
  ar_opt = \
    if defined? CONFIG and CONFIG['AR'] =~ /libtool/
      "-o" # libtool -static -o
    else
      "-r"
    end
  f.puts "\tmkdir -p build && cd build && $(CC) -O3 -std=c99 -Wall -pedantic -fPIC -c -I.. ../hat-trie/*.c && $(AR) #{ar_opt} libtries.a *.o"
end
