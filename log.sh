#! /bin/bash

SERVER='experiment'
BASE_PORT='40000'
NODE_NUM='256'

tested_nodes=0


started_netcat_array=()
for i in $(seq 1 $NODE_NUM)
do
	started_netcat_array[$i]=''
done

# run if user hits control-c
control_c()
{
	echo -e "\nExit script" 1>&2
	exit 0
}
# trap keyboard interrupt (control-c)
trap control_c SIGINT

function connection_status()
{
	#   Lines received
	# experiment.lille.senslab.info [192.168.1.110] 30001 (?) open
	# experiment.lille.senslab.info [192.168.1.110] 30256 (?) : Connection refused
	# DNS fwd/rev mismatch: experiment.lille.senslab.info != srveh
	while read line;
	do
		match=$(echo $line | grep -e "^experiment.[a-z]*.senslab.info")
		if [[ "$match" =~ "open" ]]
		then
			echo "[$(printf %03d $i)] OK"
		elif [[ "$match" =~ "Connection refused" ]]
		then
			echo "[$(printf %03d $i)] KO"
		fi
	done
}



# You can use the following trick to swap stdout and stderr.
# Then you just use the regular pipe functionality.
#
# ( proc1 3>&1 1>&2- 2>&3- ) | proc2
#
# Provided stdout and stderr both pointed to the same place at the start, this will give you what you need.
#
# What it does:
#
#     3>&1 creates a new file handle 3 which is set to the current 1 (original stdout) just to save it somewhere.
#     1>2- sets stdout to got to the current file handle 2 (original stderr) then closes 2.
#     2>3- sets stderr to got to the current file handle 3 (original stdout) then closes 3.
#
# It's effectively the swap command you see in sorting:
#
# temp   = value1;
# value1 = value2;
# value2 = temp;



# the flow is
#
#  Normal output   (STDOUT) -> (STDERR)
#         -> (STDOUT) prefixed with node number -> (STDERR)                          -> (STDOUT)
#  Connection info (STDERR) -> (STDOUT) connection_status: echo ID OK/KO
#         -> (STDERR)                           -> (STDOUT) keep 256 lines and sort  -> (STDERR)
#
# Finally there is:
#  STDOUT -> netcat normal output
#  STDERR -> connection information in order
#

for i in $(seq 1 $NODE_NUM)
do
	# $! is the PID of the last "job" forked
	{
		nc -v $SERVER $(( $BASE_PORT + $i)) 3>&1 1>&2- 2>&3- | connection_status $i ;
	} 3>&1 1>&2- 2>&3- | while read line ; do echo "[$(printf %03d $i)] $line" ; done &

	pid_array[$i]=$!
done 3>&1 1>&2- 2>&3- | head -n $NODE_NUM | sort -n 3>&1 1>&2- 2>&3-





# main() loop
while true; do read x; done

