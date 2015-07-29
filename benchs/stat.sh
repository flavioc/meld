#!/bin/sh

EXEC="${1}"
NAME="${2}"
THREADS="${3}"

if [ -z "$EXEC" ]; then
   echo "usage: stat.sh <command> <name>"
   exit 1
fi
if [ -z "$NAME" ]; then
   echo "usage: stat.sh <command> <name>"
   exit 1
fi

if [ -z "$THREADS" ]; then
   THREADS=1
fi

source $PWD/lib/common.sh
TO_RUN="${EXEC} -c th$THREADS"
stat_run "${TO_RUN}" "$NAME" th $THREADS
