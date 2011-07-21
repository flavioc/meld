
EXEC="../meld -t -f"

time_run ()
{
	$* | awk {'print $2'}
}

