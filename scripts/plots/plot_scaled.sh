#!/bin/bash

FILE="${1}"
TITLE="${2}"

if [ -z "${FILE}" ] || [ -z "${TITLE}" ]; then
   echo "Usage: plot_scaled <file> <title>"
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
TITLE="${TITLE}" FILE="${FILE}" OUTPUT="$BASE_OUTPUT.pdf" gnuplot plot_scaled.plt
sleep 0.2
pdfcrop "${BASE_OUTPUT}.pdf"
mv "${BASE_OUTPUT}-crop.pdf" "${BASE_OUTPUT}.pdf"

