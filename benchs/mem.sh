#!/bin/sh

EXEC="${1}"
NAME="${2}"
THREADS="${3}"

if [ -z "$EXEC" ]; then
   echo "usage: mem.sh <command> <name> [threads]"
   exit 1
fi
if [ -z "$NAME" ]; then
   echo "usage: mem.sh <command> <name> [threads]"
   exit 1
fi

if [ -z "$THREADS" ]; then
   THREADS=1
fi

source $PWD/lib/common.sh
if [ $THREADS -gt $CPUS ]; then
   exit 0
fi

TO_RUN="${EXEC} -c th$THREADS"
mem_run "${TO_RUN}" "$NAME" th $THREADS
