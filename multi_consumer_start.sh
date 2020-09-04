#!/bin/bash



if [ $# != 1 ] ; then 

echo "USAGE: $0 consumer_num" 

exit 1; 

fi 


num=$1
process_name='mq_multi_consumer'
log_name='mq_multi_con'

for i in $(seq 1 $num)
do
	echo $process_name$i' runing...'
	./mq_multi_consumer $log_name$i
done
