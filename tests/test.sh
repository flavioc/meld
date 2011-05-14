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

do_test ()
{
	NTHREADS=$1
	TO_RUN="${EXEC} -f ${TEST} -c ts${NTHREADS}"

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

run_test_n 1 2
run_test_n 2 5
if [ $NODES -gt 2 ]; then
	run_test_n 3 5
fi
if [ $NODES -gt 3 ]; then
	run_test_n 4 5
fi
