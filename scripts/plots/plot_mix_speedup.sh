#!/bin/bash
#
# Plots a graph using the curves and points files for mixed scheme speedups.
#

FILE="${1}"

if [ -z "${FILE}" ]; then
   echo "Usage: plot_mix_speedup.sh <base file name>"
   exit 1
fi

CURVES="${FILE}_curves"
POINTS="${FILE}_points"

if [ ! -f "${CURVES}" ] || [ ! -f "${POINTS}" ]; then
   echo "Cant find curves or points files"
   exit 1
fi

OUTPUT="$(echo $FILE | sed -e 's/\./_/g')"
OUTPUT_PDF="${OUTPUT}.pdf"

gnuplot <<EOF
set terminal postscript
set output "| ps2pdf - $OUTPUT_PDF"

load 'speedup.plt'
load 'linestyles.plt'
set style line 3 lt 3 lw 2
set style line 5 lt 5 lw 2
set pointsize 1.5
set xrange [1:16.5]
set key spacing 3
set key width 5

set multiplot

plot "${CURVES}" using 1:2 title 'MPI' w lp ls 5, \
       "${CURVES}" using 1:3 title 'DDWO' w lp ls 3, \
       "${POINTS}" using 1:2 title 'Mixed' w points pt 6 ps 2, \
       x title 'Ideal' w l ls 7

plot "${POINTS}" using 1:2:3 w labels center offset -2.0,1 font "Helvetica,13" notitle

set nomultiplot
EOF

sleep 0.2 # don't know why but this is needed for pdfcrop to succeed
pdfcrop ${OUTPUT_PDF}
mv $OUTPUT-crop.pdf $OUTPUT.pdf