#!/bin/bash

SCHEDULER="${1}"
STEP="${2}"
MIN="${3}"
MAX="${4}"

for x in ${*:5}; do
   INSTRUMENT=yes bash threads_even.sh code/$x.m $SCHEDULER $STEP $MIN $MAX 1
done
