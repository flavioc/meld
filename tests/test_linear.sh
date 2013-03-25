#!/bin/bash

SCHED=$1

run_test ()
{
   ./test.sh code/$1.m $SCHED 1 || exit 1
}

run_test linear-comp1
run_test linear-delete
run_test linear-keep1
run_test linear-keep2
run_test linear-ref-use
run_test linear-repeat
run_test dfs
run_test mfp
run_test mwst
