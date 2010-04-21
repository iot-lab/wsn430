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

node=$((30000+$1))

CMD="./tapslip6-udp -i $node -o $node -d experiment 127.0.0.1 255.0.0.0"
echo $CMD; $CMD &>tap.log &
PID=$!
sleep 1

CMD="/etc/init.d/radvd restart"
echo $CMD; $CMD

CMD="route add -A inet6 aaaa::/64 tap0"
echo $CMD; $CMD

CMD="ip -6 address add aaaa::1/64 dev tap0"
echo $CMD; $CMD

tail -n 200 -f tap.log


