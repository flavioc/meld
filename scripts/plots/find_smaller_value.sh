#!/bin/bash

FILE="${1}"
MIN=1000

if [ -z "${FILE}" ]; then
   echo "Usage: find_smaller_value.sh <file>"
   exit 1
fi

if [ ! -f "${FILE}" ]; then
   echo "Missing ${FILE}"
   exit 1
fi

first="t"

while read line; do
	if [ -n "${first}" ]; then
		first=""
		continue
	fi
	firsthere="t"
   line="`echo $line | tr -d '\r'`"
	for item in $line; do
      if [ -n "$firsthere" ]; then
         firsthere=""
         continue
      fi
      res=`echo "$item < $MIN" | bc`
      if [ "$res" == "1" ]; then
			MIN=$item
		fi
	done
done < ${FILE}

echo $MIN

