#!/bin/sh

EXEC="../meld -d"
TEST=${1}
FILE="files/$(basename $TEST .m).test"

number_nodes ()
{
	od -t u4 -N 1 -j 1 ${TEST} | head -n 1 | awk '{print $2}'
}

NODES=$(number_nodes)

do_exit ()
{
	echo $*
	exit 1
}

do_test ()
{
	NTHREADS=$1
	TO_RUN="${EXEC} -f ${TEST} -t ${NTHREADS}"

	${TO_RUN} > test.out

	DIFF=`diff -u test.out ${FILE}`
	if [ ! -z "${DIFF}" ]; then
		echo "DIFFERENCES!!!"
		echo ${DIFF}
		exit 1
	fi
	rm test.out
}

run_test_n ()
{
	NTHREADS=${1}
	TIMES=${2}
	echo "Running ${TEST} ${TIMES} times with ${NTHREADS} threads..."
	for((I=1; I <= ${TIMES}; I++)); do
		do_test ${NTHREADS}
	done
}

[ -f "${TEST}" ] || do_exit "Test code ${TEST} not found"
[ -f "${FILE}" ] || do_exit "Test file ${FILE} not found"

run_test_n 1 1
run_test_n 2 2
if [ $NODES -gt 2 ]; then
	run_test_n 3 2
fi

