#! /bin/sh
cd $(dirname $0)

SUBDIRS=$(ls -d */)


echo $(date) > compile.log

ok=0

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
		cd - > /dev/null
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

if [ $ok -eq 0 ];
then
	echo "\\033[1;32m All compilations succeded\\033[0m"
else
	echo "\\033[1;31m Compilation error\\033[0m"

fi


exit $ok


