set terminal postscript
set output "| ps2pdf - `echo $FILE`.pdf"

set title "`echo $TITLE`"

set xlabel "Workers"
set ylabel "Speedup"
set xtics (1,2,4,6,8,10,12,14,16)
set ytics (1,2,4,6,8,10,12,14,16)
set y2tics (1,2,4,6,8,10,12,14,16)
set pointsize 2

load 'linestyles.plt'

set yrange [0:16]
set y2range [0:16]
set key left top

plot "`echo $FILE`" using 1:2 smooth bezier title 'SD' w l ls 1, \
		 "`echo $FILE`" using 1:3 smooth bezier title 'ID' w l ls 2, \
		 "`echo $FILE`" using 1:4 smooth bezier title 'DD' w l ls 6, \
		 "`echo $FILE`" using 1:5 smooth bezier title 'DDWO' w l ls 3, \
		 "`echo $FILE`" using 1:6 smooth bezier title 'MPI' w l ls 5, \
		 x title 'Ideal' w l ls 7

