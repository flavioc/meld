set terminal postscript
set output "| ps2pdf - `echo $OUTPUT`"

load 'speedup.plt'
load 'linestyles.plt'
set key font "Helvetica, 23"

plot "`echo $FILE`" using 1:2 title 'GraphLab 1.0' w lp ls 1, \
		 "`echo $FILE`" using 1:3 title 'LM' w lp ls 6, \
		 x title 'Ideal' w l ls 7


