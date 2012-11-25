#!/bin/bash
#
# Plots a directory with results from instrumentation.
#

DIR="${1}"

if [ -z "${DIR}" ]; then
	echo "Usage: plot_dir.sh <directory with files>"
	exit 1
fi

plot_part ()
{
	FILE="${1}"
	SCRIPT="${2}"
   INTER="${3}"

   if [ -z "${INTER}" ]; then
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

	CMD="${INTER} ${SCRIPT} ${FILE}"
	echo -n "${SCRIPT} ${FILE}..."
	$CMD
	echo "done."
}

try_plot_part ()
{
	FILE="${1}"
	SCRIPT="${2}"

	if [ -f "${FILE}" ]; then
		plot_part "${FILE}" "${SCRIPT}"
	else
		echo "${FILE} does not exist"
	fi
}

plot_part "${DIR}/data.state" "active_inactive.py" "python"
plot_part "${DIR}/data.work_queue" "plot_quantity.sh"
plot_part "${DIR}/data.processed_facts" "plot_quantity.sh"
plot_part "${DIR}/data.sent_facts" "plot_quantity.sh"
try_plot_part "${DIR}/data.steal_requests" "plot_quantity.sh"
try_plot_part "${DIR}/data.stolen_nodes" "plot_quantity.sh"
try_plot_part "${DIR}/data.priority_queue" "plot_quantity.sh"
