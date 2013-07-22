#!/bin/sh

bench_times ()
{
	file="code/$1.m"
	bash threads_even.sh th $file
}

mpi_times ()
{
# to complete
   return 1
}

instrument ()
{
	bash instrument.sh th code/$1.m
}

my_run ()
{
	bench_times $*
	#instrument $*
}

LIST="8queens"

for prog in $LIST; do
   my_run $prog
done
