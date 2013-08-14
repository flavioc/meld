#!/bin/sh

SCHEDULER=$1
STEP=$2

bench_times ()
{
	file="code/$1.m"
	bash threads_even.sh $file $SCHEDULER $STEP
}

instrument ()
{
	bash instrument.sh th code/$1.m
}

if [ -z $STEP ]; then
   echo "usage: run.sh <scheduler> <step=even|power> programs..."
   exit 1
fi

LIST="${*:3}"

for prog in $LIST; do
   bench_times $prog
done
