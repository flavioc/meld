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
	CMD="${1}"
	DESC="${2}"

	echo -n "${DESC} "

	local time=()
	
	for((I = 0; I < ${RUNS}; I++)); do
		time[${I}]="$(time_run ${CMD})"
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
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) ${SCHEDULER} ${THREADS}"
}

do_time_mpi ()
{
	FILE="${1}"
	PROCS="${2}"

	TO_RUN="mpirun -np ${PROCS} ${EXEC} ${FILE} -c mpi"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) mpi ${PROCS}"
}

do_time_mix ()
{
	FILE="${1}"
	PROCS="${2}"
	THREADS="${3}"

	TO_RUN="mpirun -np ${PROCS} ${EXEC} ${FILE} -c mix${THREADS}"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) mix ${PROCS}/${THREADS}"
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

time_mpi ()
{
	FILE="${1}"

	do_time_mpi ${FILE} 1
	do_time_mpi ${FILE} 2
	do_time_mpi ${FILE} 4
	do_time_mpi ${FILE} 8
	do_time_mpi ${FILE} 16
}

time_mix ()
{
	FILE="${1}"
	
	do_time_mix ${FILE} 1 1
	do_time_mix ${FILE} 1 2
	do_time_mix ${FILE} 2 1
	do_time_mix ${FILE} 4 1
	do_time_mix ${FILE} 1 4
	do_time_mix ${FILE} 2 2
	do_time_mix ${FILE} 6 1
	do_time_mix ${FILE} 1 6
	do_time_mix ${FILE} 3 2
	do_time_mix ${FILE} 2 3
	do_time_mix ${FILE} 8 1
	do_time_mix ${FILE} 1 8
	do_time_mix ${FILE} 4 2
	do_time_mix ${FILE} 2 4
	do_time_mix ${FILE} 10 1
	do_time_mix ${FILE} 1 10
	do_time_mix ${FILE} 5 2
	do_time_mix ${FILE} 2 5
	do_time_mix ${FILE} 12 1
	do_time_mix ${FILE} 1 12
	do_time_mix ${FILE} 2 6
	do_time_mix ${FILE} 6 2
	do_time_mix ${FILE} 3 4
	do_time_mix ${FILE} 4 3
	do_time_mix ${FILE} 14 1
	do_time_mix ${FILE} 1 14
	do_time_mix ${FILE} 2 7
	do_time_mix ${FILE} 7 2
	do_time_mix ${FILE} 16 1
	do_time_mix ${FILE} 1 16
	do_time_mix ${FILE} 2 8
	do_time_mix ${FILE} 8 2
	do_time_mix ${FILE} 4 4
	do_time_mix ${FILE} 18 1
	do_time_mix ${FILE} 1 18
	do_time_mix ${FILE} 9 2
	do_time_mix ${FILE} 2 9
	do_time_mix ${FILE} 3 6
	do_time_mix ${FILE} 6 3
	do_time_mix ${FILE} 20 1
	do_time_mix ${FILE} 1 20
	do_time_mix ${FILE} 10 2
	do_time_mix ${FILE} 2 10
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
