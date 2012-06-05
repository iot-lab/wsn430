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
	# experiment.lille.senslab.info [192.168.1.110] 40001 (?) open
	# experiment.lille.senslab.info [192.168.1.110] 40256 (?) : Connection refused
	# DNS fwd/rev mismatch: experiment.lille.senslab.info != srveh
	while read line;
	do
		match=$(echo $line | grep -e "^experiment.[a-z]*.senslab.info")
		node_id=$(echo $match | sed 's/experiment.[a-z]*.senslab.info \[.*\] 40\([0-9]\{3\}\).*/\1/')
		if [[ "$match" =~ "open" ]]
		then
			echo "($node_id) Connected"
		elif [[ "$match" =~ "Connection refused" ]]
		then
			echo "($node_id) KO"
		fi
	done
}


# You can use the following trick to swap stdout and stderr.
# Then you just use the regular pipe functionality.
#
# ( proc1 3>&1 1>&2- 2>&3- ) | proc2


function prefixed_lines_netcat()
{
	node=$1
	nc -v $SERVER $(( $BASE_PORT + $node)) |
	while read line
	do
		echo "[$(printf %03d $node)] $line"
	done

}

function run_netcats_v2()
{
	for i in $(seq 1 $NODE_NUM)
	do
		prefixed_lines_netcat $i &
	done
}

function start_log()
{
	echo Connecting... >&2
	{ run_netcats_v2  3>&1 1>&2- 2>&3- | connection_status | head -n $NODE_NUM | sort ;} 3>&1 1>&2- 2>&3-
}

start_log | while read line; do echo "$line"; done

# main() loop
while true; do read x; done

