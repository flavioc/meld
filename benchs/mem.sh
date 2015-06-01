#!/bin/sh

EXEC="${1}"
NAME="${2}"

if [ -z "$EXEC" ]; then
   echo "usage: mem.sh <command> <name>"
   exit 1
fi
if [ -z "$NAME" ]; then
   echo "usage: mem.sh <command> <name>"
   exit 1
fi

source $PWD/lib/common.sh
TO_RUN="${EXEC} -c th1"
mem_run "${TO_RUN}" "$NAME" th 1
