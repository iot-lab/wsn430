#!/bin/sh


for node in {1..10}
do
	gnome-terminal --geometry 50x10 -e "ssh burindes@grenoble.senslab.info" --hide-menubar -t "node $node"
	# sleep 0.5
done
