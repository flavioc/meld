#!/bin/bash

FILE="${1}"
TITLE="${2}"

if [ -z "${FILE}" ] || [ -z "${TITLE}" ]; then
   echo "Usage: plot_speedup <file> <title>"
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

echo "Generating ${FILE}.pdf..."
BASE_OUTPUT="$(echo $FILE | sed -e 's/\./_/g')"
TITLE="${TITLE}" FILE="${FILE}" OUTPUT="$OUTPUT.pdf" gnuplot plot_speedup.plt
sleep 0.1
pdfcrop "${BASE_OUTPUT}.pdf"
mv "${BASE_OUTPUT}-crop.pdf" "${BASE_OUTPUT}.pdf"
