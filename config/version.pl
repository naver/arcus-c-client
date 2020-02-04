#!/usr/bin/perl
#
# arcus-c-client : Arcus C client
# Copyright 2010-2014 NAVER Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

use warnings;
use strict;

# LIBMEMCACHED VERSION
my $libmemcached_version = "0.53";

# ARCUS VERSION
# (e.g. 1.6.0_3_g15cc2db)
my $arcus_describe = `git describe`;
chomp $arcus_describe;

unless ($arcus_describe =~ m/^\d+\.\d+\.\d+/) {
    $arcus_describe = 'unknown';
}

$arcus_describe =~ s/-/_/g;

my @arcus_versions = split /_/, $arcus_describe;

if (scalar @arcus_versions > 1) {
    # discard commit number
    pop(@arcus_versions);
}

my $arcus_version = join('_', @arcus_versions);

# VERSION_NUMBER
#my $version = $libmemcached_version."-arcus-".$arcus_version;
write_file('m4/version.m4', "m4_define([VERSION_NUMBER], [$arcus_version])\n");

sub write_file {
    my $file = shift;
    my $data = shift;
    open(my $fh, "> $file") or die "Can't open $file: $!";
    print $fh $data;
    close($fh);
}
