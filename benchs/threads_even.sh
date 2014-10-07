#!/bin/bash

FILE="${1}"
SCHEDULER="${2}"
STEP="${3}"
MIN="${4}"
MAX="${5}"
RUNS="${6}"

if [ -z "${SCHEDULER}" ] || [ -z "${FILE}" ]; then
	echo "Usage: threads_even.sh <file> <scheduler> [step=even|power] [min=1] [max=32] [runs=3]"
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
if [ ! -r "${FILE}" ]; then
   echo "Code file $FILE does not exist."
   exit 1
fi

source $PWD/lib/common.sh

do_run ()
{
	NUM_THREADS="${1}"
	TO_RUN="${EXEC} ${FILE} -c ${SCHEDULER}${NUM_THREADS} -- ${MELD_ARGS}"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m)" ${SCHEDULER} ${NUM_THREADS}
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

echo -n "`basename ${FILE} .m` $SCHEDULER" >> $RESULTS_FILE
if [ "$STEP" = "even" ]; then
   for x in 1 2 4 6 8 10 12 14 16 20 24 28 32; do
      run_thread $x
   done
elif [ "$STEP" = "power" ]; then
   for x in 1 2 4 8 16; do
      run_thread $x
   done
else
   echo "Unrecognized step $STEP"
   exit 1
fi
echo >> $RESULTS_FILE

