set terminal postscript
set output "| ps2pdf - `echo $OUTPUT`"

set xlabel "Threads" font "Helvetica,25" offset 0,-1
set ylabel "Coord Speedup" font "Helvetica,25"
set y2label "Thread Speedup" font "Helvetica,25"
set xtics (1,2,4,6,8,10,12,14,16)
set ytics (1, 2) nomirror
set pointsize 2
set yrange [0:2]
set key left top
set key spacing 2
set key font "Helvetica, 25"
load 'linestyles.plt'
set key font "Helvetica, 15"
set key width 4
set pointsize 2
set y2range [0:16]
set y2tics (1, 2, 4, 6, 8, 10, 12, 14, 16)

plot "`echo $FILE`" using 1:4 axes x1y2 title 'Coord' w lp ls 2, \
		 "`echo $FILE`" using 1:6 axes x1y2 title 'Coord NO WS' w lp ls 1, \
		 "`echo $FILE`" using 1:5 title 'Coord Improve' w lp ls 4, \
		 "`echo $FILE`" using 1:7 title 'Coord NO WS Improve' w lp ls 5, \
      1


