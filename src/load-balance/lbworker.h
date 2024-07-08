#ifndef LBWORKER_H
#define LBWORKER_H
#include"lbhelper.h"
#include<fstream>
#include"./../distribute/synchronous.h"
#define HEARTBEAT_LIVENESS 3 // 3-5 is reasonable 
#define HEARTBEAT_INTERVAL 1000 // msec 
#define INTERVAL_INIT 1000 // Initial reconnect 
#define INTERVAL_MAX 8000 // After exponential backoff
class LBWorker{
    public:
        //完整的ip 例如 "tcp://localhost:4567".此处只需要提供ip信息不需提供端口信息。
        // ip 样例 ："tcp://10.2.1.9" 
        //端口为默认绑定 ： hbworker 为 6000, tkworker 为6001
        LBWorker() = default;
        LBWorker(std::string sockip){
            m_tkPool.init();
            //m_hbPool.init();
            m_context = zmq_ctx_new();
            m_hbworker = zmq_socket(m_context, ZMQ_DEALER);
            s_set_id(m_hbworker, m_hbidentity);
            std::cout<<"identity : "<<m_hbidentity<<std::endl;
            std::string hbip = sockip + std::string(":6000");
            int rcHb = zmq_connect(m_hbworker, hbip.c_str());
            //int rcHb = zmq_connect(m_hbworker, "tcp://localhost:6000");
            assert(rcHb == 0);
        }

        ~ LBWorker(){
            zmq_close(m_hbworker);
            zmq_ctx_destroy(m_context);
        }
        
        //按照一定频率发送本机信息。该方法与heartbeat的目的稍有不同。该方法是为了按照频率更新本机的资源信息
        //样例代码中的while(1) 有点沙雕，后续改成socket监听回调吧
        //因为zmq不是线程安全的，所以要避免多个线程共享一个socket。所以分两个线程：heartbeat线程，task线程
        //分别监听两个socket。
        //
        //注意：两个socket的identity不同。所以在进行心跳的时候需要在body中放入tkworker的identity信息。。这部分改了，tkworker是动态创建的。

        //hb_routine 是在程序一运行就detach到后台运行的
        //tk_routine 则是需要主程序不断调用的，主程序负责感知broker的任务分配行为，然后调用tk_routine
        //但是有可能前一个任务还没接收完，后一个任务的请求就到达了。这样两者的接收会发生错误。所以每次调用tk_routine需要创建新的线程
        //和socket来进行文件传输。需要使用线程池。
        //
        //这部分，broker在发送信息之前最好先探测每个线程是否在进行数据接受

        //开始直接调用该函数即可
        void EventLoop(){
            HBRoutine();
        }
        void HBRoutine(){
            size_t liveness = HEARTBEAT_LIVENESS;
            size_t interval = INTERVAL_INIT;
            // Send out heartbeats at regular intervals 
            uint64_t heartbeat_at = clock_time () + HEARTBEAT_INTERVAL;
            zmq_send(m_hbworker, "HBREQ", 5, 0);
            //zmq_send(m_hbworker, m_hbidentity, 10, 0);
            while (interval <= INTERVAL_MAX || liveness <= 0) {
                //std::cout<<"Still  Running !"<<std::endl;
                // Send heartbeat message to queue 
                if (clock_time () > heartbeat_at) {
                    heartbeat_at = clock_time () + HEARTBEAT_INTERVAL;
                    zmq_send(m_hbworker, "HBREQ", 5, 0);
                    std::cout<<"send heartbeat "<<std::endl;
                } 
                zmq_pollitem_t items [] = { { m_hbworker, 0, ZMQ_POLLIN, -1 }};
                int rc = zmq_poll (items, 1, HEARTBEAT_INTERVAL);
//                int rc = zmq_poll (items, 1, -1);
                if (items [0].revents & ZMQ_POLLIN){
                    string message(s_recv(m_hbworker));
                    //std::cout<<"message :"<<message<<std::endl;
                    // Receive any message from queue 
                    liveness = HEARTBEAT_LIVENESS;
                    interval = INTERVAL_INIT;
                    if(message == "HB_OK"){
                        std::cout<<"HB_OK"<<std::endl;
                        continue;
                    }
                    //这里可能会阻塞住！要注意
//                    if(message == "SKREQ"){
                    else{
                        //动态创建链接
//                        string clientIp(s_recv(m_hbworker));
//                        std::cout<<"创建链接 "<<"Target client ip : "<<clientIp<<std::endl;
//                        m_tkPool.submit(std::bind(&LBWorker::CreateSocket, this, std::placeholders::_1), clientIp);
                        std::cout<<"创建链接 "<<"Target client ip : "<<message<<std::endl;
                        m_tkPool.submit(std::bind(&LBWorker::CreateSocket, this, std::placeholders::_1), message);
                    }
                }
                else{
                    --liveness;
                    usleep(interval * 1000);  //usleep本身是微妙级别，先改成ms级别
                    interval *= 2;
                }
            }
            std::cout<<"device detached !"<<std::endl;
        }
    private:
        
        //动态创建链接
        void CreateSocket(std::string clientIp){
            //随机获取一个空闲port,动态port范围是1024～5000。
            //将ip发送给broker,指定具体client的逻辑在broker里面实现了
            
            //获取port的逻辑：从1024到5000依次调用bind，如果返回值为-1，则表明当前接口非空闲，否则绑定成功。
//            int port = 0;
//            void *newSock = zmq_socket(m_context, ZMQ_REP);
//            std::string ipPre("tcp://localhost:");
//            for(int i = 1024; i <= 5000; ++i){
//                string ip = ipPre + std::to_string(port);
//                int rc = zmq_bind(newSock, (ip.c_str()));
//                if(rc == 0){
//                    port = i;
//                    break;
//                }
//            }
//            if(port == 0){
//                std::cout<<"No available port !"<<std::endl;
//                return;
//            }
//            //获取本地ip信息，将ip与port拼接后通过broker发送到client端。
//            std::string ip = get_ip();
//            std::string workerIp = "tcp://" + ip + to_string(port);
//            zmq_send(m_hbworker, clientId.c_str(), clientId.length(), ZMQ_SNDMORE);
//            zmq_send(m_hbworker, "", 0, ZMQ_SNDMORE);
//            zmq_send(m_hbworker, workerIp.c_str(), workerIp.length(),0);

            std::cout<<"Targer client ip : "<<clientIp<<std::endl;
            void *newSock = zmq_socket(m_context, ZMQ_REP);
            int rc = zmq_connect(newSock, (clientIp.c_str()));
            assert(rc == 0);
            ServerSyn(newSock);
            zmq_close(newSock);
        
        }

//        int ServerSyn(void* socket){
////            string id(StringRecv(socket));
////            zmq_send(socket, "HELLO", 5, 0);
//            std::cout<<"Say hello to client "<<std::endl;
//            string id(s_recv(socket));
//            string taskId =string("./../../model/re") + id;
//            cout<<taskId<<endl;
//
//            zmq_msg_t reply;
//            zmq_msg_init_size (&reply, 7);
//            cout<<"DATA_OK"<<endl;
//            memcpy (zmq_msg_data (&reply), "DATA_OK", 7);
//            zmq_msg_send (&reply, socket, 0);
//
//            //文件接收后将任务摘要保存在数据库里,并且要与本机文件区分开.加一个前缀re区分
//
//            //创建文件夹,保存数据
//            int isCreate = mkdir(taskId.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
//            ReceiveFile(socket,(taskId + "/data.npy").c_str());
//            ReceiveFile(socket,(taskId + "/model.h5").c_str());
//            /*
//             * 信息维护
//             * */
//            redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
//            if(context->err) {
//                redisFree(context);
//                printf("connect redisServer err:%s\n", context->errstr);
//                return -1;
//            }
//
//            string cmdPre = "LPUSH receive-task ";
//
//            string cmd = cmdPre + id ;
//            redisReply *rdreply = (redisReply *)redisCommand(context, cmd.c_str());
//
//            freeReplyObject(rdreply);
//            printf("%s execute success\n", cmd.c_str());
//            redisFree(context);
//            return 0;
//
//
//        }


        //ThreadPool m_hbPool; //虽说hb线程中只涉及单message的往返通信，不会涉及太多的IO。但是会有对metric table的更新操作。而更新操作可能会消耗较多时间，所以此处也采用线程池处理。因为redis本身是单线程+IO多路服用，所以redis本身是线程安全的，不需要进行多余的同步控制
        ThreadPool m_tkPool; //采用reactor模式处理
        void *m_context;
        void *m_hbworker; //heartbeat

        char m_hbidentity[9];

};
#endif
