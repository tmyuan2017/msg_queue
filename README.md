# msg_queue
## 消息队列
### 预分配内存的消息队列实现：
####      1、使用ObtainMsg()从内存块队列中获取消息内存块，内存块默认4080，有16字节是消息头占用。
####   2、填入消息内容。
####   3、调用SendMsg()成员函数发送消息。
####   4、在接收线程使用RecvMsg()。接收消息，当有消息时会返回消息首地址
####   5、接收线程使用FreeMsg()成员函数释放获取到的消息，将内存块放回内存块队列中
