
number_cpus () {
   getconf _NPROCESSORS_ONLN
}

INSTRUMENT_DIR=$PWD/instrumentation_files
if [ -z "$CPUS" ]; then
   CPUS=$(number_cpus)
fi

time_run ()
{
	$1 2> "output/${2}" | awk {'print $2'}
}

mem_run ()
{
	CMD="${1} ${EXTRA_ARGS}"
   FILENAME="${2}"
   SCHEDULER="${3}"
   NUM_THREADS="${4}"
	DESC="${FILENAME} ${SCHEDULER}${NUM_THREADS}"

   if [ -f "args/${FILENAME}" ]; then
      source "args/${FILENAME}"
      CMD="${CMD} ${MELD_ARGS} -- ${PROG_ARGS}"
   fi

   mkdir -p output
	echo -n "${DESC} ..."
   contents=$($CMD | tail -n 1)
   tput cub 3
   echo "$contents"
}

stat_run ()
{
	CMD="${1} ${EXTRA_ARGS}"
   FILENAME="${2}"
   SCHEDULER="${3}"
   NUM_THREADS="${4}"
	DESC="${FILENAME} ${SCHEDULER}${NUM_THREADS}"

   if [ -f "args/${FILENAME}" ]; then
      source "args/${FILENAME}"
      CMD="${CMD} ${MELD_ARGS} -- ${PROG_ARGS}"
   fi

   mkdir -p output
	echo "${DESC}"
   $CMD
}

time_run_n ()
{
	CMD="${1} ${EXTRA_ARGS}"
   FILENAME="${2}"
   SCHEDULER="${3}"
   NUM_THREADS="${4}"
	DESC="${FILENAME} ${SCHEDULER}${NUM_THREADS}"
   OUTPUT_FILE="${FILENAME}_${SCHEDULER}${NUM_THREADS}"

   if [ -f "args/${FILENAME}" ]; then
      source "args/${FILENAME}"
      CMD="${CMD} ${MELD_ARGS} -- ${PROG_ARGS}"
   fi

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

compile_test () {
   BT="$1"
   CPP="code/$BT.cpp"
   TARGET="build/$BT"
   mkdir -p build
   if [ ! -f "$CPP" ]; then
      echo "Cannot find file $CPP"
      exit 1
   fi
   if [ -f "$TARGET" ]; then
      if [ "$TARGET" -nt "$CPP" ]; then
         return
      fi
   fi
   echo -n "=> Compiling $BT.cpp into $TARGET..."
   CURRENT_DIR="$PWD"
   cd ..
   make clean 2>&1 > /dev/null || exit 1
   make PROGRAM="benchs/$CPP" -j$CPUS target 2>&1 > /dev/null || exit 1
   echo -en "\r\033[K"
   mv target $CURRENT_DIR/$TARGET
   cd $CURRENT_DIR
}

ensure_vm () {
   if [ -x "../meld" ]; then
      return
   fi
   CURRENT_DIR="$PWD"
   echo -n "=> Compiling the virtual machine..."
   cd ..
   make clean 2>&1 > /dev/null || exit 1
   make -j$CPUS 2>&1 > /dev/null || exit 1
   echo -en "\r\033[K"
   cd $CURRENT_DIR
}
