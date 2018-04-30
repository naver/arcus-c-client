#!/bin/bash

if [[ -d .git || -f .git || ! -f m4/version.m4 ]]
then
  perl config/version.pl || die "Failed to run config/version.pl"
fi

autoreconf --install --force --verbose -Wall
