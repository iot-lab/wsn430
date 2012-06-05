#! /bin/bash


# Configuration
# Default configuration should be enough
SERVER='experiment'
BASE_PORT='40000'
NODE_NUM='256'


#
# Basic functions:
#
# Simple functions to aggregate and prefix the log lines with node id
#

function start_formatted_logs()
{
	# 'while read' is required here too, instead the lines get mixed up
	run_background_netcats 2> /dev/null | while read line; do echo "$line"; done
}

# Start all the connections to the nodes in background
function run_background_netcats()
{
	for i in $(seq 1 $NODE_NUM)
	do
		netcat_prefixed $i &
	done
}

# $1: node number
function netcat_prefixed()
{
	node=$1
	nc -v $SERVER $(( $BASE_PORT + $node)) |
	while read line
	do
		echo "[$(printf %03d $node)] $line"
	done
}




#
# Advanced functions:
#
# More complex functions that analyse the connection informations and
# format the output to tell to which nodes we were able to connect.
#


# Output logs from nodes in format "[NODE_ID] node_output_line" on STDOUT
# Output connexion information in format "(NODE_ID) Connexion_info" on STDERR
function start_advanced_logs()
{
	echo Connecting... >&2
	# 'while read' is required here too, instead the lines get mixed up
	start_log | while read line; do echo "$line"; done
}


# format the logs and the connexion informations
function start_log()
{
	# Swap STDOUT and STDERR
	# ( proc1 3>&1 1>&2- 2>&3- ) | proc2
	{
		{
			run_background_netcats  3>&1 1>&2- 2>&3- ;
		} | nodes_connection_status | head -n $NODE_NUM | sort ;
	} 3>&1 1>&2- 2>&3-
}


# Interpret and format connection information received from verbose netcat
function nodes_connection_status()
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




# Simple script that only aggregates the serial port and prefix them with "[NODE_ID] "
#   start_formatted_logs

# Advanced script that format the connection information
start_advanced_logs





# Block until SIGINT == Ctrl+C

control_c()
{
	echo -e "\nExit script" 1>&2
	exit 0
}
trap control_c SIGINT
while true; do read DUMMY; done

