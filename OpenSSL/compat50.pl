open(FILE, '<e_os.h');
@data = <FILE>;
close(FILE);
open(FILE, '>e_os.h');
for(@data)
{
	print FILE $_;
}
print FILE "#undef AI_PASSIVE\n";
close(FILE);
exit(0);
