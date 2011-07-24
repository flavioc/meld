for i in `seq 2 $NUMCOLS`; do
	POSITION=`echo "$SLICE * ($i-1)" | bc -l`
	PLOTSTR="$PLOTSTR\n
	plot ($POSITION-$INTERVAL) w line ls 1 notitle\n
	plot ($POSITION+$INTERVAL) w line ls 1 notitle"
done
