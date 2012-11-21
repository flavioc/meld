#!/bin/bash

. ./common.sh
. ./lines.sh

BIGGER=`bash ./find_bigger_value.sh ${FILE}`

if [ $BIGGER -eq 0 ]; then
   echo "No quantities for ${FILE}"
#exit 1
fi

for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	MUL='*'
	NOW="plot \"${FILE}\" using 1:($POSITION-$INTERVAL + ((\$$i / $BIGGER) $MUL $SIZE)) title \"thread`expr $i - 1`\" with lines"
	PLOTSTR="$PLOTSTR\n
	$NOW"
done

gnuplot <<EOF
set output "$FILE.png"
load 'base.plt'
set xrange [0:$MAXX]
set ylabel "Max: $BIGGER"

set style line 1 lt 1 lc rgb "black" lw 2
set style line 3 lt 1 lw 1
set style line 2 lt 1 lw 5

set style arrow 1 nohead ls 3 lc rgb "green"
set style arrow 2 nohead ls 2 lc rgb "red"

set multiplot

`echo -e "$PLOTSTR"`

unset multiplot
EOF

