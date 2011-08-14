#!/bin/bash

. ./common.sh
. ./lines.sh

# idle
for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" using 1:(\$$i == 0 ? $POSITION-$INTERVAL : 1/0):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 2"
	PLOTSTR="$PLOTSTR\n$NOW"
done

# active
for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" every :10 using 1:(\$$i == 1 ? $POSITION-$INTERVAL : 1/0):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 1"
	PLOTSTR="$PLOTSTR\n$NOW"
done

# sched
for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" every :10 using 1:(\$$i == 2 ? $POSITION-$INTERVAL : 1/0):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 3"
	PLOTSTR="$PLOTSTR\n$NOW"
done

# round
for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" every :10 using 1:(\$$i == 3 ? $POSITION-$INTERVAL : 1/0):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 4"
	PLOTSTR="$PLOTSTR\n$NOW"
done

# comm
for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" every :10 using 1:(\$$i == 4 ? $POSITION-$INTERVAL : 1/0):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 5"
	PLOTSTR="$PLOTSTR\n$NOW"
done

gnuplot <<EOF
set output "$FILE.pdf"
load 'base.plt'
set xrange [0:$MAXX]

set ylabel "Active/Inactive"

set style line 1 lt 1 lc rgb "black" lw 2
set style line 3 lt 1 lw 1
set style line 2 lt 1 lw 5

set style arrow 1 nohead ls 3 lc rgb "green"
set style arrow 2 nohead ls 2 lc rgb "red"
set style arrow 3 nohead ls 2 lc rgb "blue"
set style arrow 4 nohead ls 2 lc rgb "black"
set style arrow 5 nohead ls 2 lc rgb "yellow"

set multiplot

`echo -e $PLOTSTR`

unset multiplot
EOF

