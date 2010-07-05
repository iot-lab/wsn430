#!/bin/bash

SUBDIRS="01_simple_multitask 02_queue_multitask 03_semphr_synchro 04_mutex_multitask 05_network_device 06_coordinator 07_node_appli 08_sink_appli 09_adc 10_adc_sink"

echo $(date) > compile.log

ok=1

for i in $SUBDIRS
do
	cd $i
	echo $i >> ../compile.log
	make clean
	make 2>> ../compile.log
	if [ $? -eq 0 ]; then
		echo " SUCCESS" >> ../compile.log
	else
		echo " ERROR" >> ../compile.log
		ok=0
	fi
	cd - > /dev/null
done

for i in $SUBDIRS
do
	cd $i
	make clean
	cd - > /dev/null
done

cat compile.log

if [ $ok -eq 1 ]; then
	exit 0
else
	exit 1
fi
