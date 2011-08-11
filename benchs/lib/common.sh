
EXEC="../meld -t -f"

time_run ()
{
	$* | awk {'print $2'}
}

time_run_n ()
{
	CMD="${1} ${EXTRA_ARGS}"
	DESC="${2}"

	echo -n "${DESC}"

	local time=()
	
	for((I = 0; I < ${RUNS}; I++)); do
		time[${I}]="$(time_run ${CMD})"
		echo -n " ${time[$I]}"
	done

	total=0
	for((I = 0; I < ${#time[@]}; I++)); do
		total=`expr ${time[$I]} + ${total}`
	done
	average=`expr ${total} / ${#time[@]}`
	echo " -> ${average}"
}

do_time_mpi ()
{
	FILE="${1}"
	PROCS="${2}"
	THREADS="${3}"

	TO_RUN="mpirun -np ${PROCS} ${EXEC} ${FILE} -c ${SCHEDULER}${THREADS}"
	time_run_n "${TO_RUN}" "$(basename ${FILE} .m) ${SCHEDULER}${PROCS}/${THREADS}"
}

