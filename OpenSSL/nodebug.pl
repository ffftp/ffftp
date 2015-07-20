open(FILE, '<ms\ntdll.mak');
@data = <FILE>;
close(FILE);
open(FILE, '>ms\ntdll.mak');
for(@data)
{
	$_ =~ s/ \/debug//g;
	print FILE $_;
}
close(FILE);
exit(0);
