# mq
用c++实现的一个简单消息中间件
一、框架
1.Publisher:消息的生产者，向交换器Exchange发布消息，发送消息时还要指定Routing Key。
2.Exchange:交换器从生产者那收到消息后，根据Routing Key、Exchange Type和Binding key联合使用，分发消息到queue中。
3.Queue: 消息最终被送到Queue中并等待consumer取走，一个message可以被同时拷贝到多个queue中。
4.Binding&Binding Key:通过Binding将Exchange与Queue关联起来，在绑定（Binding）Exchange与Queue的同时，一般会指定一个binding key。可以将多个Queue绑定到同一个Exchange上，并且可以指定相同的Binding Key。
5.Broker:消息队列服务器。
6.Connection:网络连接，Producer和Consumer都是通过TCP连接到Server的。
7.Consumer:消息的消费者，如果有多个消费者同时订阅同一个Queue中的消息，Queue中的消息会被平摊给多个消费者。

二、功能实现
1.消息生产消费
2.exchange支持direct topic 和fanout三种模式
3.支持发布订阅模型
4.支持消息持久化
