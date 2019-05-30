#!/usr/bin/perl
#
# Example script to extract the type out of the encode/decode
# functions. This should be run on one of the generated header
# files. The generated code is pretty regular, so it should be
# safe to use a regex to 'parse' the content.
#
use strict;
use warnings;

my %encode;
my %decode;

sub banner($) {
	print ("=" x $_[0] . "\n");
}

sub extractor($$) {
	my $name = $_[0];
	my $args = $_[1];
	my $value = (split /,/, $args)[1];
	my $type = (split / +/,$value)[1];
	$type =~ s/^\s+|\s+$//g;
	# $encode{$name} = $type;
	return ($name, $type);
}

while(<>) {
	my $line = $_;
	if ($line =~ /^int (encode_.*)\((.*)\);/) {
		my ($name, $type) = &extractor($1, $2);
		$encode{$name} = $type;
	}
	if ($line =~ /^int (decode_.*)\((.*)\);/) {
		my ($name, $type) = &extractor($1, $2);
		$decode{$name} = $type;
	}
}

banner 80;
map { print "'$_' => '$encode{$_}'\n" } keys %encode;
banner 80;
map { print "'$_' => '$decode{$_}'\n" } keys %decode;
banner 80;

