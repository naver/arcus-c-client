#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

if [[ -d .git || -f .git || ! -f m4/version.m4 ]]
then
  perl config/version.pl
fi

autoreconf --install --force --verbose -Wall
