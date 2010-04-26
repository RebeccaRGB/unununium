#!/usr/bin/perl


# Read PPM file.

$sig = <>; chomp $sig;
$sizes = <>; chomp $sizes;
$cols = <>; chomp $cols;

{
	local $/;
	$data = <>;
}


# Sanity check.

$sig ne "P6" and die;
$sizes ne "72 256" and die;
$cols ne "255" and die;
(length $data) != 3 * 72 * 256 and die;


# Output header.

print "\\ GENERATED FILE DO NOT EDIT\n";

# Output data.

for my $ch (2..7) {
	for my $cl (0..15) {
		printf "\n\\ %x%x\n", $ch, $cl;
		for my $py (0..15) {
			for my $px (0..7) {
				print " " if $px == 4;
				my $hor = ($px ^ 2) + 8*($ch - 2);
				my $ver = $py + 16*$cl;
				my $wot = $hor + 72*$ver;
				my $bytes = substr($data, 3*$wot, 3);
				my $nyb = int ((ord $bytes) / 16);
				printf "%x", $nyb;
				print " ," if ($px & 3) == 3;
			}
			print "\n";
		}
	}
}
