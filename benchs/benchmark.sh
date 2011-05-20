#!/bin/bash
# Runs meld and takes execution times

EXEC="../meld -t -f"
SCHEDULER="${1}"
RUNS="${2}"

if [ -z "${SCHEDULER}" ] || [ -z "${RUNS}" ]; then
	echo "Usage. benchmark.sh <scheduler> <number of runs to avg>"
	exit 1
fi

if [ "${SCHEDULER}" = "mpi" ]; then
	IS_MPI="true"
fi
if [ "${SCHEDULER}" = "mix" ]; then
	IS_MIX="true"
fi

time_run ()
{
	$* | awk {'print $2'}
}

time_run_n ()
{
	EXEC="${1}"
	DESC="${2}"

	echo -n "${DESC} "
	
	for((I = 0; I < ${RUNS}; I++)); do
		time[${I}]="$(time_run ${EXEC})"
	done

	total=0
	str="("
	for((I = 0; I < ${#time[@]}; I++)); do
		total=`expr ${time[$I]} + ${total}`
		if [ $I -gt 0 ]; then
			str="${str}/"
		fi
		str="${str}${time[$I]}"
	done
	str="${str})"
	average=`expr ${total} / ${#time[@]}`
	echo "- ${average} ${str}"
}

do_time_thread ()
{
	FILE="${1}"
	THREADS="${2}"

	TO_RUN="${EXEC} ${FILE} -c ${SCHEDULER}${THREADS}"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) ${SCHEDULER} ${THREADS} threads"
}

time_thread ()
{
	FILE="${1}"

	do_time_thread ${FILE} 1
	do_time_thread ${FILE} 2
	do_time_thread ${FILE} 4
	do_time_thread ${FILE} 8
	do_time_thread ${FILE} 16
}

time_file ()
{
	if [ -n "${IS_MIX}" ]; then
		time_mix $*
	elif [ -n "${IS_MPI}" ]; then
		time_mpi $*
	else
		time_thread $*
	fi
}

for file in code/*.m; do
	time_file "${file}"
done
