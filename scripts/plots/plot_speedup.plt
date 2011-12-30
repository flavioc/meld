set terminal postscript
set output "| ps2pdf - `echo $OUTPUT`"

load 'speedup.plt'
load 'linestyles.plt'

plot "`echo $FILE`" using 1:2 title 'SD' w lp ls 1, \
		 "`echo $FILE`" using 1:3 title 'DD' w lp ls 6, \
		 "`echo $FILE`" using 1:4 title 'DDWO' w lp ls 3, \
       "`echo $FILE`" using 1:5 title 'MPI' w lp ls 5, \
		 x title 'Ideal' w l ls 7


