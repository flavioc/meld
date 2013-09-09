#!/bin/bash

FILE="${1}"
TITLE="${2}"
COORD="${3}"

if [ -z "${FILE}" ] || [ -z "${TITLE}" ]; then
   echo "Usage: plot_speedup <file> <title> [coord=no]"
   exit 1
fi

if [ ! -f "${FILE}" ]; then
   echo "Missing ${FILE}"
   exit 1
fi

if [ -f "${FILE}.pdf" ];then
   echo "Removing ${FILE}.pdf"
   rm -f "${FILE}.pdf"
fi

BASE_OUTPUT="$(echo $FILE | sed -e 's/\./_/g')"
echo "Generating ${BASE_OUTPUT}.pdf..."
if [ -z "${SCRIPT}" ]; then
   if [ -z "${COORD}" ]; then
      TITLE="${TITLE}" FILE="${FILE}" OUTPUT="$BASE_OUTPUT.pdf" gnuplot plot_speedup.plt
   else
      TITLE="${TITLE}" FILE="${FILE}" OUTPUT="$BASE_OUTPUT.pdf" gnuplot plot_speedup_shortest.plt
   fi
else
   TITLE="${TITLE}" FILE="${FILE}" OUTPUT="$BASE_OUTPUT.pdf" gnuplot $SCRIPT
fi
sleep 0.2
pdfcrop "${BASE_OUTPUT}.pdf"
mv "${BASE_OUTPUT}-crop.pdf" "${BASE_OUTPUT}.pdf"
