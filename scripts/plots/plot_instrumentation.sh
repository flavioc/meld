#!/bin/bash
#
# Constructs plots for a complete instrumentation directory.

DIR="${1}"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z "${DIR}" ]; then
   echo "Usage: plot_instrumentation.sh <dir>"
   exit 1
fi

if [ ! -d "${DIR}" ]; then
   echo "${DIR} does not exist."
   exit 1
fi

echo "<html><head><title>Instrumentation</title></head><body><ol>" > $DIR/index.html

for test in ${DIR}/*; do
   if [ -d "${test}" ]; then
      tbase=$(basename $test)
      echo "<li>$tbase<ul>" >> $DIR/index.html
      for cpu in ${test}/*; do
         ncpu=$(basename $cpu)
         if [ -f "$cpu/data.state" ]; then
            echo "<li><a href=\"$tbase/$ncpu/index.html\">$ncpu threads</a></li>" >> $DIR/index.html
            bash $SCRIPT_DIR/plot_dir.sh ${cpu} $tbase $ncpu
         fi
      done
      echo "</ul></li>" >> $DIR/index.html
   fi
done
echo "</ol></body></html>" >> $DIR/index.html
