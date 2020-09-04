LIB_HOME=./libnnext.a
FILE_HOME=/home/dev/code/god_trunk/libsrc
LIBS += $(LIB_HOME) 
INC = -I $(FILE_HOME) -I $(FILE_HOME)/comm 

all:mq_normal_producer mq_normal_consumer1 mq_normal_consumer2 mq_ack_producer mq_ack_consumer mq_multi_producer mq_priority_consumer mq_multi_consumer mq_priority_producer mq_pull_consumer mq_durable_producer mq_durable_consumer mq_conSrv mq_logicSrv mq_persisSrv
mq_normal_producer:mq_util.o client.o message.o ini_file.o normal_producer.o logger.o 
	g++ -g -Wall -std=c++11 -o mq_normal_producer mq_util.o client.o message.o  ini_file.o normal_producer.o logger.o
mq_util.o:mq_util/mq_util.h mq_util/mq_util.cpp
	g++ -c -g -Wall -std=c++11 mq_util/mq_util.h mq_util/mq_util.cpp
ini_file.o:ini_file/ini_file.h ini_file/ini_file.cpp
	g++ -c -g -Wall -std=c++11 ini_file/ini_file.h ini_file/ini_file.cpp
client.o:client/client.h client/client.cpp ini_file.o
	g++ -c -g -Wall -std=c++11 client/client.h client/client.cpp ini_file.o
message.o:message/message.h message/message.cpp
	g++ -c -g -Wall -std=c++11 message/message.h message/message.cpp
normal_producer.o:client/normal_producer.cpp
	g++ -c -g -Wall -std=c++11  client/normal_producer.cpp
logger.o:logger/logger.h logger/logger.cpp
	g++ -c -g -Wall -std=c++11 logger/logger.h logger/logger.cpp


mq_normal_consumer1:mq_util.o client.o message.o logger.o ini_file.o normal_consumer1.o
	g++ -g -Wall -std=c++11 -o mq_normal_consumer1 mq_util.o client.o message.o logger.o ini_file.o normal_consumer1.o 
normal_consumer1.o:client/normal_consumer1.cpp
	g++ -c -g -Wall -std=c++11 client/normal_consumer1.cpp

mq_normal_consumer2:mq_util.o client.o message.o logger.o ini_file.o normal_consumer2.o
	g++ -g -Wall -std=c++11 -o mq_normal_consumer2 mq_util.o client.o message.o logger.o ini_file.o normal_consumer2.o 
normal_consumer2.o:client/normal_consumer2.cpp
	g++ -c -g -Wall -std=c++11 client/normal_consumer2.cpp

mq_ack_producer:mq_util.o client.o message.o logger.o ini_file.o ack_producer.o
	g++ -g -Wall -std=c++11 -o mq_ack_producer mq_util.o client.o message.o logger.o ini_file.o ack_producer.o 
ack_producer.o:client/ack_producer.cpp
	g++ -c -g -Wall -std=c++11 client/ack_producer.cpp

mq_ack_consumer:mq_util.o client.o message.o logger.o ini_file.o ack_consumer.o
	g++ -g -Wall -std=c++11 -o mq_ack_consumer mq_util.o client.o message.o logger.o ini_file.o ack_consumer.o 
ack_consumer.o:client/ack_consumer.cpp
	g++ -c -g -Wall -std=c++11 client/ack_consumer.cpp

mq_multi_producer:mq_util.o client.o message.o logger.o ini_file.o multi_producer.o
	g++ -g -Wall -std=c++11 -o mq_multi_producer mq_util.o client.o message.o logger.o ini_file.o multi_producer.o 
multi_producer.o:client/multi_producer.cpp
	g++ -c -g -Wall -std=c++11 client/multi_producer.cpp

mq_priority_consumer:mq_util.o client.o message.o logger.o ini_file.o priority_consumer.o
	g++ -g -Wall -std=c++11 -o mq_priority_consumer mq_util.o client.o message.o logger.o ini_file.o priority_consumer.o 
priority_consumer.o:client/priority_consumer.cpp
	g++ -c -g -Wall -std=c++11 client/priority_consumer.cpp

mq_multi_consumer:mq_util.o client.o message.o logger.o ini_file.o multi_consumer.o
	g++ -g -Wall -std=c++11 -o mq_multi_consumer mq_util.o client.o message.o logger.o ini_file.o multi_consumer.o 
multi_consumer.o:client/multi_consumer.cpp
	g++ -c -g -Wall -std=c++11 client/multi_consumer.cpp

mq_priority_producer:mq_util.o client.o message.o logger.o ini_file.o priority_producer.o
	g++ -g -Wall -std=c++11 -o mq_priority_producer mq_util.o client.o message.o logger.o ini_file.o priority_producer.o 
priority_producer.o:client/priority_producer.cpp
	g++ -c -g -Wall -std=c++11 client/priority_producer.cpp

mq_pull_consumer:mq_util.o client.o message.o logger.o ini_file.o pull_consumer.o
	g++ -g -Wall -std=c++11 -o mq_pull_consumer mq_util.o client.o message.o logger.o ini_file.o pull_consumer.o
pull_consumer.o:client/pull_consumer.cpp
	g++ -c -g -Wall -std=c++11 client/pull_consumer.cpp

mq_durable_producer:mq_util.o client.o message.o logger.o ini_file.o durable_producer.o
	g++ -g -Wall -std=c++11 -o mq_durable_producer mq_util.o client.o message.o logger.o ini_file.o durable_producer.o 
durable_producer.o:client/durable_producer.cpp
	g++ -c -g -Wall -std=c++11 client/durable_producer.cpp

mq_durable_consumer:mq_util.o client.o message.o logger.o ini_file.o durable_consumer.o
	g++ -g -Wall -std=c++11 -o mq_durable_consumer mq_util.o client.o message.o logger.o ini_file.o durable_consumer.o
durable_consumer.o:client/durable_consumer.cpp
	g++ -c -g -Wall -std=c++11 client/durable_consumer.cpp

mq_conSrv:timer.o shm_queue.o sem_lock.o mq_util.o logger.o clientConnect.o message.o ini_file.o connectServer.o 
	g++ -g -Wall -std=c++11 -o mq_conSrv timer.o shm_queue.o sem_lock.o mq_util.o logger.o clientConnect.o message.o ini_file.o connectServer.o
timer.o:./timer/timer.h ./timer/timer.cpp
	g++ -c -g -Wall -std=c++11 ./timer/timer.h ./timer/timer.cpp
shm_queue.o:shm_queue/shm_queue.h shm_queue/shm_queue.cpp
	g++ -c -g -Wall -std=c++11 shm_queue/shm_queue.h shm_queue/shm_queue.cpp
sem_lock.o:sem_lock/sem_lock.h sem_lock/sem_lock.cpp
	g++ -c -g -Wall -std=c++11 sem_lock/sem_lock.h sem_lock/sem_lock.cpp
clientConnect.o:connectServer/clientConnect.h connectServer/clientConnect.cpp
	g++ -c -g -Wall -std=c++11 connectServer/clientConnect.h connectServer/clientConnect.cpp
connectServer.o:connectServer/connectServer.h connectServer/connectServer.cpp
	g++ -c -g -Wall -std=c++11 connectServer/connectServer.h connectServer/connectServer.cpp

mq_logicSrv: timer.o shm_queue.o sem_lock.o mq_util.o logger.o message.o ini_file.o logicServer.o
	g++ -g -Wall -std=c++11 -o mq_logicSrv timer.o shm_queue.o sem_lock.o mq_util.o logger.o message.o ini_file.o logicServer.o 
logicServer.o:logicServer/logicServer.h logicServer/logicServer.cpp
	g++ -c -g -Wall -std=c++11 logicServer/logicServer.h logicServer/logicServer.cpp

mq_persisSrv: timer.o shm_queue.o sem_lock.o mq_util.o logger.o message.o ini_file.o PersistenceServer.o
	g++ -g -Wall -std=c++11 -o mq_persisSrv timer.o shm_queue.o sem_lock.o mq_util.o logger.o message.o ini_file.o PersistenceServer.o
PersistenceServer.o:PersistenceServer/PersistenceServer.h PersistenceServer/PersistenceServer.cpp
	g++ -c -g -Wall -std=c++11 PersistenceServer/PersistenceServer.h PersistenceServer/PersistenceServer.cpp

clean :
	rm  *.o client/client.h.gch connectServer/connectServer.h.gch connectServer/clientConnect.h.gch logger/logger.h.gch logicServer/logicServer.h.gch message/message.h.gch mq_util/mq_util.h.gch PersistenceServer/PersistenceServer.h.gch sem_lock/sem_lock.h.gch shm_queue/shm_queue.h.gch timer/timer.h.gch ini_file/ini_file.h.gch