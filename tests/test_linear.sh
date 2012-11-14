#!/bin/bash

SCHED=$1

run_test ()
{
   ./test.sh code/$1.m $SCHED || exit 1
}

run_test linear-comp1
