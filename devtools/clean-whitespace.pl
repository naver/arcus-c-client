#!/usr/bin/perl
use strict;
use FindBin qw($Bin);

chdir "$Bin/.." or die;
my @files = `git ls-files` or die;

foreach my $f (@files) {
    chomp($f);
    next if $f =~ /\.png$/;
    open(my $fh, $f) or die;
    my $before = do { local $/; <$fh>; };
    close ($fh);
    my $after = $before;
    $after =~ s/\t/    /g;
    $after =~ s/ +$//mg;
    $after .= "\n" unless $after =~ /\n$/;
    next if $after eq $before;
    open(my $fh, ">$f") or die;
    print $fh $after;
    close($fh);
}
