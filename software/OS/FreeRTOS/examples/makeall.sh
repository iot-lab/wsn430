#!/bin/bash

SUBDIRS="\
01_simple_multitask \
02_queue_multitask \
03_semphr_synchro \
04_mutex_multitask \
05_tdma_node_adc \
06_tdma_sink_adc \
07_starnet_node \
08_starnet_sink \
09_monitor_appli_node \
10_monitor_appli_sink"


echo $(date) > compile.log

ok=0
echo "arguments: $@"

for i in $SUBDIRS
do
	cd $i
	echo $i >> ../compile.log
	make $@ clean
	make $@ 2>> ../compile.log
	if [ $? -eq 0 ]; then
		echo " SUCCESS $i" >> ../compile.log
	else
		echo " ERROR $i" >> ../compile.log
		ok=1
	fi
	cd - > /dev/null
done

for i in $SUBDIRS
do
	cd $i
	make $@ clean
	cd - > /dev/null
done

cat compile.log

if [[ $ok -eq 0 ]];
then
	echo -e "\\033[1;32m All compilations succeded\\033[0m"

else
	echo -e "\\033[1;31m Compilation error\\033[0m"

fi
exit $ok
