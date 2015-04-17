#!/bin/bash

EXEC="${1}"
NAME="${2}"
SCHEDULER="${3}"
STEP="${4}"
MIN="${5}"
MAX="${6}"
RUNS="${7}"

if [ -z "${SCHEDULER}" ]; then
	echo "Usage: threads_even.sh <exec> <name> <scheduler> [step=even|power] [min=1] [max=32] [runs=3]"
	exit 1
fi

if [ -z "${MIN}" ]; then
	MIN=1
fi
if [ -z "${MAX}" ]; then
	MAX=32
fi
if [ -z "${STEP}" ]; then
   STEP=even
fi
if [ -z "${RUNS}" ]; then
   RUNS=3
fi

source $PWD/lib/common.sh
if [ -z $RESULTS_FILE ]; then
   RESULTS_FILE=/dev/null
fi

do_run ()
{
	NUM_THREADS="${1}"
	TO_RUN="${EXEC} -t -c ${SCHEDULER}${NUM_THREADS}"
   if [ ! -z "$INSTRUMENT" ]; then
      dir="$INSTRUMENT_DIR/$NAME/$NUM_THREADS"
      mkdir -p $dir
      TO_RUN="$TO_RUN -i $dir/data"
   fi
	time_run_n "${TO_RUN}" "$NAME" ${SCHEDULER} ${NUM_THREADS}
}

run_thread ()
{
   NUM_THREADS="${1}"
   if [ $MIN -le $x ]; then
      if [ $MAX -ge $x ]; then
         if ! do_run $x; then
            echo "Error..."
            exit 1
         fi
         echo -n " $BENCHMARK_TIME" >> $RESULTS_FILE
      fi
   fi
}

echo -n "$NAME $SCHEDULER" >> $RESULTS_FILE
if [ "$STEP" = "even" ]; then
   for x in 1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32; do
      run_thread $x
   done
elif [ "$STEP" = "power" ]; then
   for x in 1 2 4 8 16 24 32; do
      run_thread $x
   done
else
   echo "Unrecognized step $STEP"
   exit 1
fi
echo >> $RESULTS_FILE

