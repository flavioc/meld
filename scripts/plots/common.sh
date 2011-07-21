
FILE="${1}"

if [ -z "${FILE}" ]; then
	echo "Please tell me the file name."
	exit 1
fi

NUMCOLS=`head -n 2 ${FILE} | tail -n 1 | awk -F' ' ' { print NF }'`
MAXX=`tail -n 1 ${FILE} | awk {'print $1'}`

SLICE=`echo "1.0 / ($NUMCOLS)" | bc -l`
INTERVAL=`echo "$SLICE / 3.5" | bc -l`
SIZE=`echo "2.0 * $INTERVAL" | bc -l`

PLOTSTR=""

