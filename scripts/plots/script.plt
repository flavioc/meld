set terminal postscript
set output "| ps2pdf - `echo $FILE`.pdf"

set title "`echo $TITLE`"

set xlabel "Workers"
set ylabel "Speedup"
set xtics (1,2,4,6,8,10,12,14,16)
set pointsize 2

set style line 1 lt 1 lw 8
set style line 2 lt 2 lw 4
set style line 3 lt 3 lw 6
set style line 5 lt 5 lw 6
set style line 6 lt 4 lw 6
set style line 4 lt 4 lw 3
set style line 7 lt 1 lw 2

set yrange [1:18]
set y2range [1:18]
set y2tics
set key left top

plot "`echo $FILE`.txt" using 1:2 smooth bezier title 'GSD' w l ls 1, \
		 "`echo $FILE`.txt" using 1:3 smooth bezier title 'LSD' w l ls 2, \
		 "`echo $FILE`.txt" using 1:4 smooth bezier title 'LDD' w l ls 6, \
		 "`echo $FILE`.txt" using 1:5 smooth bezier title 'LDDWO' w l ls 3, \
		 "`echo $FILE`.txt" using 1:6 smooth bezier title 'MPI' w l ls 5, \
		 x title 'Linear' w l ls 7

