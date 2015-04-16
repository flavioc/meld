#!/bin/sh

file=$1
if [ -z "$file" ]; then
   exit 1
fi

if [ -z "$CLEAN" ]; then
   make clean
fi
make PROGRAM=$file target $2 || exit 1
newfile="$(dirname $file)/$(basename $file .cpp)"
echo "==> $newfile"
mv target $newfile
