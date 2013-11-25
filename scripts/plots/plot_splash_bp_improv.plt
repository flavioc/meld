set terminal postscript
set output "| ps2pdf - `echo $OUTPUT`"

load 'speedup.plt'
load 'linestyles.plt'
set yrange [0:5]
set y2range [0:5]
set ytics (1,2,3,4,5)
set y2tics (1,2,3,4,5)
set key font "Helvetica, 23"
set key right top

plot "`echo $FILE`" using 1:2 title 'GraphLab 1.0' w lp ls 1, \
		 "`echo $FILE`" using 1:3 title 'LM' w lp ls 6


