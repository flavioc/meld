#!/bin/bash

. ./common.sh
. ./lines.sh

for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" using 1:(\$$i < 1 ? $POSITION-$INTERVAL : 1/0):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 2"
	PLOTSTR="$PLOTSTR\n$NOW"
done

for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	NOW="plot \"${FILE}\" every :10 using 1:(\$$i < 1 ? 1/0 : $POSITION-$INTERVAL):(0):($INTERVAL + $INTERVAL)
			title \"thread`expr $i - 1`\" with vectors arrowstyle 1"
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

set multiplot

`echo -e $PLOTSTR`

unset multiplot
EOF

