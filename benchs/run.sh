#!/bin/sh

bench_times ()
{
	file="code/$1.m"
	bash threads_even.sh th $file
}

instrument ()
{
	bash instrument.sh th code/$1.m
}

my_run ()
{
	#bench_times $*
	#instrument $*
}

#my_run neural_network_32_16p
