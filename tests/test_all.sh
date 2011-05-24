#!/bin/bash

for file in code/*.m; do
	./test.sh $file ${1} || exit 1
done
