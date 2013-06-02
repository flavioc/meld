#!/bin/bash

SCHED=$1

run_test ()
{
   ./test.sh code/$1.m $SCHED 1 || exit 1
}

run_test linear-delete
run_test linear-keep1
run_test linear-keep2
run_test linear-ref-use
run_test linear-repeat
run_test linear-comp1
run_test pagerank-linear-sync
run_test visit
run_test dfs
run_test mfp
run_test mwst
run_test shortest_path1
run_test shortest_path2
run_test right_connectivity1
run_test pagerank1
run_test pagerank2
run_test all_pairs1
run_test all_pairs2
run_test bp1
run_test neural_network1
