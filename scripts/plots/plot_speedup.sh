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
TITLE="${TITLE}" FILE="${FILE}" gnuplot plot_speedup.plt
