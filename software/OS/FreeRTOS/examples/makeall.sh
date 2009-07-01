#!/bin/bash

SUBDIRS="01_simple_multitask 02_queue_multitask 03_semphr_synchro 04_mutex_multitask 05_network_device 06_coordinator 07_node_appli 08_sink_appli"

echo $(date) > compile.log

for i in $SUBDIRS
do
	cd $i
	make clean
	make
	if [ $? = 0 ]; then
		echo "$i success" >> ../compile.log
	else
		echo "$i error" >> ../compile.log
	fi
	cd - > /dev/null
done

cat compile.log
