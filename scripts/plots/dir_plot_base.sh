#!/bin/bash
#
# Takes a directory as argument and generates plots as PDF files for each stuff.* files in the directory.

DIR="${1}"

if [ -z "${DIR}" ]; then
   echo "Usage: ${0}.sh <dir>"
   exit 1
fi

if [ ! -d "${DIR}" ]; then
   echo "${DIR} is not a directory."
   exit 1
fi

if [ ! -f "plot_${what}.sh" ]; then
   echo "Missing plot_${what}.sh"
   exit 1
fi

build_cool_name ()
{
   arg="${1}"
   arg="${arg#${what}.}"
   arg="${arg//_/ }"
   ## capitalize each word
   echo $arg | sed -e 's/^./\U&/' -e 's/ [a-z]/ \U&/g'
}

for file in ${DIR}/${what}.*; do
   base="$(basename $file)"
   ext="${base: -3}"
   if [ "$ext" == "pdf" ]; then
      continue
   fi
   name="$(build_cool_name ${base})"
   bash plot_${what}.sh $file "${name}"
done
