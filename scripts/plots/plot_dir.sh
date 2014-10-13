#!/bin/bash
#
# Plots a directory with results from instrumentation.
#

DIR="${1}"
PROG="${2}"
CPU="${3}"
HTML="$DIR/index.html"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z "${DIR}" ]; then
	echo "Usage: plot_dir.sh <directory with files>"
	exit 1
fi

plot_part ()
{
	FILE="${1}"
	SCRIPT="${2}"
   TITLE="${3}"

   if [[ $SCRIPT == *.py ]]; then
      INTER="python"
   else
      INTER="bash"
   fi

	if [ -z "${SCRIPT}" ]; then
		echo "Missing script."
		exit 1
	fi

	if [ ! -f "${FILE}" ]; then
		echo "Cannot find file ${FILE}."
		exit 1
	fi

   if [[ ! -f $FILE.png ||  $FILE -nt $FILE.png ]]; then
      CMD="${INTER} $SCRIPT_DIR/${SCRIPT} ${FILE}"
      echo -n "Processing ${FILE}..."
      $CMD
      if [ $? -eq 0 ]; then
         echo "<h2>$TITLE</h2><img src=\"$(basename $FILE).png\" />" >> $HTML
         echo "done."
      else
         echo "fail."
      fi
   fi
}

try_plot_part ()
{
	FILE="${1}"
	SCRIPT="${2}"
   TITLE="${3}"

	if [ -f "${FILE}" ]; then
		plot_part "${FILE}" "${SCRIPT}" "${TITLE}"
	else
		echo "${FILE} does not exist"
	fi
}

echo "<html><head><title>$PROG - $CPU threads</title></head><body>" > $HTML

plot_part "${DIR}/data.state" "active_inactive.py" "State"
plot_part "${DIR}/data.bytes_used" "plot_quantity.sh" "Bytes Used"
plot_part "${DIR}/data.node_lock_ok" "plot_quantity.sh" "Node Locks (First Ok)"
plot_part "${DIR}/data.node_lock_fail" "plot_quantity.sh" "Node Locks (First Failed)"
plot_part "${DIR}/data.derived_facts" "plot_quantity.sh" "Derived Facts"
plot_part "${DIR}/data.consumed_facts" "plot_quantity.sh" "Consumed Facts"
plot_part "${DIR}/data.rules_run" "plot_quantity.sh" "Rules Run"
plot_part "${DIR}/data.sent_facts_same_thread" "plot_quantity.sh" "Sent Facts (in same thread)"
plot_part "${DIR}/data.sent_facts_other_thread" "plot_quantity.sh" "Sent Facts (to other thread)"
plot_part "${DIR}/data.sent_facts_other_thread_now" "plot_quantity.sh" "Sent Facts (to other thread now, indexing included)"
try_plot_part "${DIR}/data.stolen_nodes" "plot_quantity.sh" "Stolen Nodes"
try_plot_part "${DIR}/data.priority_nodes_thread" "plot_quantity.sh" "Set Priority (in same thread)"
try_plot_part "${DIR}/data.priority_nodes_others" "plot_quantity.sh" "Set Priority (in another thread)"

echo "</body></html>" >> $HTML
