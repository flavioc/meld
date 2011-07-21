#!/bin/bash

FILE="${1}"
MAX=0

first="t"

while read line; do
	if [ -n "${first}" ]; then
		first=""
		continue
	fi
	firsthere="t"
	for x in $line; do
		if [ -n "${firsthere}" ]; then
			firsthere=""
			continue
		fi
		if [ $x -gt $MAX ]; then
			MAX=$x
		fi
	done
done < ${FILE}

echo $MAX

