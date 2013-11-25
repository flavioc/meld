set terminal postscript
set output "| ps2pdf - `echo $OUTPUT`"

load 'speedup.plt'
load 'linestyles.plt'

plot "`echo $FILE`" using 1:2 title 'Threads' w lp ls 1, \
		 "`echo $FILE`" using 1:3 title 'Coord' w lp ls 6, \
		 x title 'Ideal' w l ls 7


