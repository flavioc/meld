#!/bin/bash

SCHEDULER="${1}"
FILE="${2}"

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ]; then
	echo "Usage: instrument.sh <scheduler> <file>"
	exit 1
fi

source $PWD/lib/common.sh

DATA_DIR=$PWD/tmp_instrumentation_files

mkdir -p $DATA_DIR

FILEBASENAME=`basename $FILE`
SUBDIR="${DATA_DIR}/${FILEBASENAME}"
mkdir -p $SUBDIR

do_run ()
{
	NUM_THREADS="${1}"
	SUBSUBDIR="${SUBDIR}/${SCHEDULER}/${NUM_THREADS}"
	mkdir -p $SUBSUBDIR

	TO_RUN="${EXEC} ${FILE} -c ${SCHEDULER}${NUM_THREADS} -i $SUBSUBDIR/data"
	echo -n "$FILEBASENAME sched:${SCHEDULER} ${NUM_THREADS} "
	TIME=`time_run ${TO_RUN}`
	echo $TIME
}

do_run 4
do_run 8
do_run 16

