#! /bin/bash

SERVER='experiment'
BASE_PORT='40000'

pid_array=()

control_c()
# run if user hits control-c
{
	echo -e "\nExit script" 1>&2
	exit 0
}
# trap keyboard interrupt (control-c)
trap control_c SIGINT



for i in $(seq 1 256)
do
	# $! is the PID of the last "job" forked
	nc $SERVER $(( $BASE_PORT + $i)) &

	pid_array[$i]=$!
done


# main() loop
while true; do read x; done

