#!/bin/bash


if [ $# != 2 ] ; then 

echo "USAGE: $0 producer_num message_len" 

exit 1; 

fi 

num=$1
len=$2
process_name='mq_multi_producer'
log_name='mq_multi_prod'

for i in $(seq 1 $num)
do
	echo $process_name$i' runing...'
	./mq_multi_producer $len $log_name$i
done
