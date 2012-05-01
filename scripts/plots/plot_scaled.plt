set terminal postscript
set output "| ps2pdf - `echo $OUTPUT`"

load 'speedup.plt'
load 'linestyles.plt'

set xlabel "Workers (Dataset Size)" font "Helvetica,25" offset 0,-1
set ylabel "Execution Time" font "Helvetica,25"

set noytics
set noy2tics
set auto y
set auto y2
set yrange [0:]
set key right bottom
set ytics autofreq
set y2tics autofreq
set key width 6

plot "`echo $FILE`" using 1:2 title 'SD' w lp ls 1, \
		 "`echo $FILE`" using 1:3 title 'DD' w lp ls 6, \
		 "`echo $FILE`" using 1:4 title 'DDWO' w lp ls 3, \
       "`echo $FILE`" using 1:5 title 'MPI' w lp ls 5

