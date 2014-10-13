#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

. $SCRIPT_DIR/common.sh
. $SCRIPT_DIR/lines.sh

BIGGER=`python $SCRIPT_DIR/find_max_value.py ${FILE}`
TOTAL=`python $SCRIPT_DIR/find_total_value.py ${FILE}`

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

WIDTH=800
HEIGHT=600

if [ $NUMCOLS -gt 24 ]; then
   WIDTH=1000
   HEIGHT=800
fi

gnuplot <<EOF
set output "$FILE.png"
load '$SCRIPT_DIR/base.plt'
set terminal png size $WIDTH,$HEIGHT
set xrange [0:$MAXX]
set ylabel "Max: $BIGGER \n Total: $TOTAL"

set style line 1 lt 1 lc rgb "black" lw 2
set style line 3 lt 1 lw 1
set style line 2 lt 1 lw 5

set style arrow 1 nohead ls 3 lc rgb "green"
set style arrow 2 nohead ls 2 lc rgb "red"

set multiplot

`echo -e "$PLOTSTR"`

unset multiplot
EOF

