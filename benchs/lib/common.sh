
EXEC="../meld -t -f"

time_run ()
{
	$1 2> "output/${2}" | awk {'print $2'}
}

time_run_n ()
{
	CMD="${1} ${EXTRA_ARGS}"
   FILENAME="${2}"
   SCHEDULER="${3}"
   NUM_THREADS="${4}"
	DESC="${FILENAME} ${SCHEDULER}${NUM_THREADS}"
   OUTPUT_FILE="${FILENAME}_${SCHEDULER}${NUM_THREADS}"

   mkdir -p output
	echo -n "${DESC}"

	local time=()
	
	for((I = 0; I < ${RUNS}; I++)); do
      echo -n " $I/$RUNS"
		time[${I}]=$(time_run "${CMD}" "${OUTPUT_FILE}")
      tput cub 3
		echo -n "${time[$I]}"
	done

	total=0
	for((I = 0; I < ${#time[@]}; I++)); do
		total=`expr ${time[$I]} + ${total}`
	done
	average=`expr ${total} / ${#time[@]}`
	echo " -> ${average}"
   export BENCHMARK_TIME=${average}
}
