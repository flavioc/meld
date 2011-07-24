set terminal postscript
set output "| ps2pdf - `echo $FILE`.pdf"

set notitle

set x2label "Workers"
set ylabel "Efficiency"
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

plot "`echo $FILE`" using 1:2 smooth bezier title 'SD' w l ls 1, \
		 "`echo $FILE`" using 1:3 smooth bezier title 'ID' w l ls 2, \
		 "`echo $FILE`" using 1:4 smooth bezier title 'DD' w l ls 6, \
		 "`echo $FILE`" using 1:5 smooth bezier title 'DDWO' w l ls 3