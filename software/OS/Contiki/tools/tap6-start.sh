#!/bin/bash

USAGE="usage: $0 <node number>"

PID=-1
control_c()
# run if user hits control-c
{
  echo -en "\n*** Ouch! Exiting ***\n"
  /etc/init.d/radvd stop
  kill $PID
  echo "script stopped"
  exit 0
}


# trap keyboard interrupt (control-c)
trap control_c SIGINT

if [ $# -ne 1 ]; then
	echo $USAGE
	exit 1
fi
echo $1 | grep -Eq "^(\+|-)?[0-9]+$"
if [ $? -ne 0 ] ; then
	echo $USAGE
	echo -e "\tnode number must be a positive integer 1"
	exit 1
fi


if [ $1 -le 0 ]; then
	echo $USAGE
	echo -e "\tnode number must be a positive integer"
	exit 1
fi

node=$1

CMD="./senslip6 -n $node"
echo $CMD; $CMD &>tap.log &
PID=$!
sleep 1

CMD="/etc/init.d/radvd restart"
echo $CMD; $CMD

CMD="ifconfig tap0 add aaaa::1/64"
echo $CMD; $CMD

tail -n 200 -f tap.log
