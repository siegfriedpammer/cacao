#! perl -w

# This scripts finds calls of printf and printf-like functions which
# do not use a literal string as format. Such calls are potentially
# dangerous as they could use formats which do not match their
# argument lists.

use strict;
use File::Find;
no warnings 'File::Find';

my @FUNCTIONS =
    (
     # NAME ........ name of function
     # FORMATPOS ... index of format string argument, first is 1
     {NAME => 'printf'     ,FORMATPOS => 1},
     {NAME => 'sprintf'    ,FORMATPOS => 2},
     {NAME => 'fprintf'    ,FORMATPOS => 2},
     {NAME => 'error'      ,FORMATPOS => 1},
     {NAME => 'dolog'      ,FORMATPOS => 1},
     {NAME => 'dolog_plain',FORMATPOS => 1},
    );

my $rxall;
my %tests;
my @tests;
my @files;

prepare();
find(\&wanted,'.');
process();

sub prepare
{
    for my $f (@FUNCTIONS) {
	my $name = $f->{NAME};
	$rxall .= "|" if $rxall;
	$rxall .= "\\b$name\\b";
#	push @{$tests{$f->{FORMATPOS}}},$f;
	$tests{$f->{NAME}} = $f;
    }
    $rxall = qr/($rxall)/;
    study $rxall;
}

sub wanted
{
    push @files, $File::Find::name if /\.[ch]$/i;
    push @files, $File::Find::name if /\.inc$/i;
}

sub get_args
{
    my ($coderef,$file,$line) = @_;
    my $arg;
    my $level = 0;
    my @args;

 ARG:
    while (1) {
	unless ($$coderef =~ /\G(.*?)([,()"])/g) {
	    return undef;
	}
	$arg .= $1;
	my $d = $2;
	if ($d eq ')') {
	    if ($level-- == 0) {
		push @args,$arg;
		return \@args;
	    }
	}
	elsif ($d eq ',') {
	    if ($level == 0) {
		push @args,$arg;
		$arg = '';
		next ARG;
	    }
	}
	elsif ($d eq '(') {
	    $level++;
	}
	elsif ($d eq '"') {
	    unless($$coderef =~ /\G  (  ([^\\"]|\\.)*   "  )  /gx) {
		return undef;
	    }
	    $d .= $1;
	}
	$arg .= $d;
    }
}

sub process
{
    for my $file (@files) {
#	print "$file\n";
	open(FILE,"<$file") or die "Could not open $file: $!";
	my $code;
	my $pos = 0;
	my @pos;
	while (<FILE>) {
	    s/\n/ /;
	    s/[ \t]+/ /;
	    $code .= $_;
	    push @pos,$pos;
	    $pos += length;
	}
	close(FILE);

	my $line = 1;
    OCC:
	while ($code =~ /$rxall/g) {
	    my $pos = pos($code);
	    my $fname = $1;
	    my $test = $tests{$fname};
	    while (pos($code) > $pos[$line]) {$line++}
#	    print "$1\n";
	    unless ($code =~ /\G\s*\(/g) {
		print STDERR "$file:$line: warning: could not find parenthesis\n";
		pos($code) = $pos;
		next OCC;
	    }
	
	    my $index = 1;

	    my $args = get_args(\$code,$file,$line);
	    unless ($args) {
		print STDERR "$file:$line:warning: could not parse arguments\n";
		pos($code) = $pos;
		next OCC;
	    }

	    if (@$args < $test->{FORMATPOS}) {
		print STDERR "$file:$line: warning: could not find format of $fname\n";
		next OCC;
	    }

	    my $fmt = $args->[$test->{FORMATPOS} - 1];

	    if ($fmt =~ /^\s*$/) {
		print "$file:$line: missing format: $fname(".join(',',@$args).")\n";
		next OCC;
	    }

	    unless ($fmt =~ /^\s*"/) {
		print "$file:$line: nonliteral format: $fname(".join(',',@$args).")\n";
		next OCC;
	    }

	    if ($fmt =~ /^\s*"(.*)"\s*$/) {
		my $f = $1;
		$f =~ s/"\s+"//g;
		my $pct = 0;
		while ($f =~ /(%%?)/g) {$pct++ if length($1)==1};

		if ($pct != @$args - $test->{FORMATPOS}) {
		print "$file:$line: arguments?: $fname(".join(',',@$args).")\n";
		}
	    }
	}
    }
}

