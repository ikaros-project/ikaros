#!/usr/bin/perl

#
# validate.pl
#
# Perl script to test all xml files in the IKAROS directory tree
# All error and warning messages are generated and the number of
# files that generate errors (but not warning) are counted
#
# The script assumes that the working directory is the location
# of the script
#
# Set $runall to 1 to not only test but to run all examples
# This will take a very long time
#
# Christian Balkenius 2003-04-30
#

use File::Find;
use File::Path;
use File::Copy;
use File::Basename;

use Cwd;
    
# Set source and dest paths

#$sourcepath = $ARGV[0];

$ikarospath	= getcwd . "/../Bin/ikaros";
$sourcepath	= "../Source";
$runall		= 0;

print "Running all test test files in $sourcepath\n" if $runall;
print "Validating Ikaros test files in $sourcepath\n" if !$runall;

# Set file suffixes for files to validate

@suffixlist = ('\.ikc');

# Traverse the directory tree

$filecount = 0;
$errcount = 0;

find(\&process, $sourcepath);


if($errcount != 0)
{
	print "Total errors: $errcount  ($filecount files tested).\n";
}
else
{
	print "There were no errors! ($filecount files tested)\n";
}

# Process each file in the tree

sub process
{
	($name, $path, $suffix) = fileparse($File::Find::name, @suffixlist);

	# Skip special directories

	if($name eq "build")
	{
		($File::Find::prune = 1);	
		return;
	}

	if($name eq "UserModules")
	{
		($File::Find::prune = 1);	
		return;
	}

	# Skip if suffix not in list

	return if $suffix eq "";

	# Skip class ikc files

	if(!($name =~ m/_test$/))
	{
		($File::Find::prune = 1);	
		return;
	}

	# Validate the ikc file

	$sourcefile = $File::Find::name;
	$filecount++;

	$syscode = system($ikarospath, "-v", $sourcefile) if $runall;
	$syscode = system($ikarospath, "-q", "-s0", $sourcefile) if !$runall;

	print "\t $syscode\n";
	
	if($syscode == -1)
	{
		print "Validated $sourcefile: FAILED.\n";
		return;
	}

	$exit_value = $? >> 8;
	$signal_num = $? & 127;

	if($exit_value == 0 && $signal_num == 0)
	{
		print "Validated \"$sourcefile\": OK.\n";
		return;
	}

	$errcount++;

	if($exit_value != 0)
	{
		print "Validated \"$sourcefile\": EXIT_CODE = $exit_value.\n";
		return;
	}

	print "Validated \"$sourcefile\": SIGNAL_NO = $signal_num.\n";
}
