open(FILE, '<makefile');
@data = <FILE>;
close(FILE);
open(FILE, '>makefile');
for(@data)
{
	$_ =~ s/ \/debug//g;
	$_ =~ s/libssl-1_1-x64/ssleay32/g;
	$_ =~ s/libcrypto-1_1-x64/libeay32/g;
	$_ =~ s/libssl-1_1/ssleay32/g;
	$_ =~ s/libcrypto-1_1/libeay32/g;
	$_ =~ s/\$\(MTOUTFLAG\)ssleay32\.dll/\$\(MTOUTFLAG\)ssleay32\.dll;#2/g;
	$_ =~ s/\$\(MTOUTFLAG\)libeay32\.dll/\$\(MTOUTFLAG\)libeay32\.dll;#2/g;
	print FILE $_;
}
close(FILE);
exit(0);
