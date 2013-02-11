#!/bin/bash
#
# Constructs plots for a complete instrumentation directory.

DIR="${1}"

if [ -z "${DIR}" ]; then
   echo "Usage: plot_instrumentation.sh <dir>"
   exit 1
fi

if [ ! -d "${DIR}" ]; then
   echo "${DIR} does not exist."
   exit 1
fi

for test in ${DIR}/*; do
   if [ -d "${test}" ]; then
      for sched in ${test}/*; do
         if [ -d "${sched}" ]; then
            for cpu in ${sched}/*; do
               echo "$cpu/data.state"
               if [ -f "$cpu/data.state" ]; then
                  bash plot_dir.sh ${cpu}
               fi
               for stealing in ${cpu}/*; do
                  if [ -d "$stealing" ]; then
                     if [ -f "$stealing/data.state" ]; then
                        bash plot_dir.sh ${stealing}
                     fi
                  fi
               done
            done
         fi
      done
   fi
done
