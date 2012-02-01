#!/bin/bash


USAGE="usage: $0 <node number>"
SENSLIP_PID=-1
NODE=-1

# run if user hits control-c
control_c()
{
	echo -en "\n*** Ouch! Exiting ***\n"
	/etc/init.d/radvd stop
	kill $SENSLIP_PID
	echo "script stopped"
	exit 0
}


# get the node ID from command line
get_args()
{
	if [ $# -ne 1 ]; then
		echo $USAGE
		exit 1
	fi

	echo $1 | grep -Eq "^(\+|-)?[0-9]+$"	 # match numbers
	# Non number or number <= 0
	if [ $? -ne 0 ] || [ $1 -le 0 ]  ; then
		echo $USAGE
		echo -e "\tnode number must be a positive integer"
		exit 1
	fi
	NODE=$1
}

check_connectivity()
{
	PORT=$(( 30000 + $1 )) 

	# Test if the connection to the node is already used
	# If a process is already connected to this node, netcat can connect but quits quickly


	echo -n .
	(echo "" | nc experiment ${PORT} -q 2) &> /dev/null &
	PID=$!
	sleep 1
	echo -n .

	jobs -l | grep ${PID} | grep -q Running
	if [ $? -eq 0 ];
	then
		wait ${PID}
	else
		echo "Cannot connect to experiment:${PORT}" >&2
		echo "Please kill all the connections to the serial link of the node" >&2
		echo "     even the direct connections using experiment:40000+<REAL_NODE_ID>" >&2

		exit 1
	fi
}


# trap keyboard interrupt (control-c)
trap control_c SIGINT

get_args $@
echo -n "Checking connectivity to node serial link"
check_connectivity $1
echo

# start being verbose, read each command
set -v
set -e

sysctl -x net.ipv6.conf.all.forwarding=1

# create the tunnel over serial link on /dev/tap0
./senslip6 -n ${NODE} | tee tap.log &
SENSLIP_PID=$!
sleep 1


# set a link local IPv6 address for /dev/tap0
/etc/init.d/radvd restart

# set a global address for tap0
ifconfig tap0 add aaaa::1/64

# print new IPv6 global address
ifconfig tap0

echo "Kill the program with Ctrl+c"
wait ${SENSLIP_PID}
