set terminal postscript
set output "| ps2pdf - `echo $FILE`.pdf"

set notitle

set x2label "Workers" font "Helvetica,20"
set ylabel "Efficiency" font "Helvetica,20"
set x2tics (1,2,4,6,8,10,12,14,16)
set y2tics (0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0)
set ytics (0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0)
set pointsize 2

load 'linestyles.plt'

set yrange [`echo $SMALLER`:1]
set y2range [`echo $SMALLER`:1]
set xrange [1:16]
set x2range [1:16]
set key left bottom
set key box
set key spacing 2
set key width 5
set key font "Helvetica, 20"
set pointsize 2.5

plot "`echo $FILE`" using 1:2 title 'SD' w lp ls 1, \
		 "`echo $FILE`" using 1:3 title 'DD' w lp ls 6, \
		 "`echo $FILE`" using 1:4 title 'DDWO' w lp ls 3, \
		 "`echo $FILE`" using 1:5 title 'MPI' w lp ls 5
