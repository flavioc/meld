#!/bin/bash

FILE="${1}"
TITLE="${2}"

if [ -z "${FILE}" ] || [ -z "${TITLE}" ]; then
   echo "Usage: plot_efficiency.sh <file> <title>"
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

smaller=$(bash ./find_smaller_value.sh ${FILE})
res=`echo "$smaller > 0.5" | bc`
if [ "$res" == "1" ]; then
   smaller="0.5"
else
   smaller="0.0"
fi
echo "Generating ${FILE}.pdf..."
SMALLER="$smaller" TITLE="${TITLE}" FILE="${FILE}" gnuplot plot_efficiency.plt
