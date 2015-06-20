#!/bin/sh
find ../ -name "*[chp]" > ./cscope.files
ctags --extra=+q -L ./cscope.files -f ./tags
rm ./cscope.files
