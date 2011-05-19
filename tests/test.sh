#!/bin/sh

EXEC="../meld -d"
TEST=${1}
FILE="files/$(basename $TEST .m).test"

NODES=$(sh ./number_nodes.sh $TEST)

do_exit ()
{
	echo $*
	exit 1
}

run_diff ()
{
	TO_RUN="${1}"
	${TO_RUN} > test.out
	DIFF=`diff -u ${FILE} test.out`
	if [ ! -z "${DIFF}" ]; then
		echo "DIFFERENCES!!!"
		diff -u ${FILE} test.out
		exit 1
	fi
	rm test.out
}

do_test ()
{
	NTHREADS=${1}
	SCHED=${2}
	TO_RUN="${EXEC} -f ${TEST} -c ${SCHED}${NTHREADS}"

	run_diff "${TO_RUN}"
}

do_test_mpi ()
{
	NPROCS=${1}

	TO_RUN="mpirun -n ${NPROCS} ${EXEC} -f ${TEST} -c mpi"

	run_diff "${TO_RUN}"
}

run_test_n ()
{
	NTHREADS=${1}
	TIMES=${2}
	SCHED=${3}
	echo "Running ${TEST} ${TIMES} times with ${NTHREADS} threads (SCHED: ${SCHED})..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_test ${NTHREADS} ${SCHED}
	done
}

run_test_mpi_n ()
{
	NPROCS=${1}
	TIMES=${2}
	echo "Running ${TEST} ${TIMES} times with ${NPROCS} processes (SCHED: mpi)..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_test_mpi ${NPROCS}
	done
}

[ -f "${TEST}" ] || do_exit "Test code ${TEST} not found"
[ -f "${FILE}" ] || do_exit "Test file ${FILE} not found"

loop_sched ()
{
	SCHED=${1}
	run_test_n 1 1 ${SCHED}
	run_test_n 2 1 ${SCHED}
	if [ $NODES -gt 2 ]; then
		run_test_n 3 1 ${SCHED}
	fi
	if [ $NODES -gt 3 ]; then
		run_test_n 4 1 ${SCHED}
	fi
	if [ $NODES -gt 4 ]; then
		run_test_n 5 1 ${SCHED}
	fi
	if [ $NODES -gt 5 ]; then
		run_test_n 6 1 ${SCHED}
	fi
	if [ $NODES -gt 6 ]; then
		run_test_n 7 1 ${SCHED}
	fi
	if [ $NODES -gt 7 ]; then
		run_test_n 8 1 ${SCHED}
	fi
}

loop_sched_mpi ()
{
	run_test_mpi_n 1 1
	run_test_mpi_n 2 1
	
	if [ $NODES -gt 2 ]; then
		run_test_mpi_n 3 1
	fi
	if [ $NODES -gt 3 ]; then
		run_test_mpi_n 4 1
	fi
	if [ $NODES -gt 4 ]; then
		run_test_mpi_n 5 1
	fi
	if [ $NODES -gt 5 ]; then
		run_test_mpi_n 6 1
	fi
	if [ $NODES -gt 6 ]; then
		run_test_mpi_n 7 1
	fi
	if [ $NODES -gt 7 ]; then
		run_test_mpi_n 8 1
	fi
}

loop_sched ts
loop_sched tl
loop_sched td
loop_sched_mpi

