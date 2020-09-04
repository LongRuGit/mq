#!/bin/bash


if [ $# != 1 ] ; then 

echo "USAGE: $0 sig_val" 

exit 1; 

fi 

sig_val=$1

process_name='mq_multi_producer'

pid=$(ps -ef | grep $process_name | grep -v grep | awk '{print $2}')

for i in $pid;
do
	kill $sig_val $i
	if [ $? != 0 ]; then
		echo $i' stop failed'
	else
		echo $i' stop succeed'
	fi
done
