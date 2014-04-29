#!/bin/sh

if [ -d .git ]
then
  perl config/version.pl || die "Failed to run config/version.pl"
fi

autoreconf --install --force --verbose -Wall
