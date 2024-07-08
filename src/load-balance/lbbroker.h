#ifndef LBBROKER_H
#define LBBROKER_H
#include"lbhelper.h"
#include<fstream>
#include"./../thread-pool/thread-pool.h"

/*
 * 包含两个方向的数据流向：
 * 1. client --- broker ：client 先发送任务的元信息，broker收到信息后根据自身的metric table选取相应的worker（先实现个简单的，按照时间戳）。将后续数据发往worker端。
 * 2. broker --- worker ：
 *  a) hb感知流：接收worker的心跳包，更新metric table中的键值存活世间。（超时未感知就直接删除）
 *  b) tk任务分配执行流：将client端数据传入相应worker，将worker的结果返回到相应client
 *                                                                                                                                    ..
 * 
 * 针对每条数据流的分析：
 * client --- broker 数据流： 
 *   问题 ：1. 目前broker只有一个入口，但是可能会有多个client同时发送请求。请求是单个数据包，但是文件的传输会有连续的数据。如果多个client进程共享这一个socket的话，经过测试会发生数据错乱。
 *   方案1 ： broker在第一次接收client任务时，检查client id（这里假设client在发送时采用单线程发送，也就是同一时间段只有一个任务在发送状态），如果client id不存在redis中，表明是第一次发送。为其分配相应的worker，并在数据库内插入 client --- broker键值对。 后续数据包到达后检查client id，若redis中包含相应键值对，则只进行转发。当发送结束时，删除redis中的相应键值对。
 *   方案2 ： broker在第一次接收client任务时，检查id，此id为client节点id与任务id的组合id，用于对同一client的任务进行唯一标识。broker在IO线程池中挑选一个线程并为其分配临时socket，将临时端口返回client。二者通过此唯一信道传输信息。 
 *   线程池 + Reactor(目前暂时没有对线程池做额外封装，当请求量大于线程总数时，驳回),线程池的封装会用到多线程编程中第三节对线程安全栈的封装（可以看一看，是个亮点）
 *
 * broker --- worker 数据流：
 * */
class LBBroker{
    public:
        LBBroker(std::string sockip){
            m_hbPool.init();
            m_tkPool.init();
            m_context = zmq_ctx_new();
            m_tkBroker = zmq_socket(m_context, ZMQ_ROUTER);
            assert(m_tkBroker);
            m_hbBroker = zmq_socket(m_context, ZMQ_ROUTER);
            assert(m_hbBroker);
            std::string tkip = sockip + std::string(":6001");
          /*
           * 这里tk的流程需要修改，此处tk socket负责监听client的任务请求。当tk threads有空闲时，需要与worker的tk socket动态建立链接
           * 这里的动态是指按照当前系统端口使用情况，broker为新tk thread分配端口并将端口信息通知到worker处，woker再进行socket的绑定。
           * */
            std::string hbip = sockip + std::string(":6000");
            std::cout<<hbip<<std::endl;
            int rcTk = zmq_bind(m_tkBroker, tkip.c_str());
            //int rcTk = zmq_bind(m_tkBroker, "tcp://127.0.0.1:6001");
            assert(rcTk == 0);
            //int rcHb = zmq_bind(m_hbBroker, hbip.c_str());
            int rcHb = zmq_bind(m_hbBroker, "tcp://*:6000");
//            int rcHb = zmq_connect(m_hbBroker, (const char *)(sockip + std::string(":6000")));
            assert(rcHb == 0);
        }
        /*
         * 作为heart beat的Reactor，采用event loop的方式接收hb message，并在hb线程池中选取线程对象更新metric table
         * 作为task的Reactor，采用event loop的方式接收task 元信息，并在tk线程池中选取线程对象根据metric table中的worker信息挑选最合适的worker。
         * 完成后续的任务转发
         * 二者共用一个loop
         * */ 
        void EventLoop(){
            //uint64_t heartbeat_at = clock_time () + HEARTBEAT_INTERVAL;
            //zmq_send(m_hbworker, m_hbidentity, 10, 0);
            //            zmq_msg_t ping;
            //            zmq_msg_init_size(&ping, 10);
            //            memcpy(zmq_msg_data(&data), m_hbidentity, 10);
            //            zmq_msg_send(hbworker, )
            while (true) {
                std::cout<<"开始感知 "<<std::endl;
                zmq_pollitem_t items [] = { { m_hbBroker, 0, ZMQ_POLLIN, 0 },{m_tkBroker, 0, ZMQ_POLLIN, 0} };
                int rc = zmq_poll (items, 2, -1);
                if (items [0].revents & ZMQ_POLLIN){
                    // 心跳包到达
                    // 补充数据格式
                    std::cout<<"收到心跳"<<std::endl;
                    std::string workerHbId (s_recv(m_hbBroker));

//                    {
//                        // Second frame is empty
//                        std::string empty = s_recv(m_hbBroker);
//                        assert(empty.size() == 0);
//                    }

                    // Third frame is READY or else a client reply address
                    std::string hbreq(s_recv(m_hbBroker));

                    //接收到的是worker端tk_routine的identity
                    
                    // 补充条件判断,补充线程分配
                    m_hbPool.submit(std::bind(&LBBroker::HbGetHandle, this, std::placeholders::_1), workerHbId);
                    // worker的心跳包到达
                }
                /*
                 * 原先针对tkBroker有两条逻辑：
                 * 1. 发送元信息得到候选worker的id
                 * 2. 向候选worker发起链接申请
                 * 两条逻辑在broker处由SKREQ进行区分。
                 *
                 * 先版本只有一条逻辑因此broker里不用再使用SKREQ了，但是worker里仍需要该标志
                 * */
                if(items[1].revents & ZMQ_POLLIN){
                    // client的任务包到达
                    //address empty request三个frame
                    std::string clientId (s_recv(m_tkBroker));

                    {
                        // Second frame is empty
                        std::string empty = s_recv(m_tkBroker);
                        assert(empty.size() == 0);
                    }

                    // Third frame is READY or else a client reply address
                    std::string taskInfo(s_recv(m_tkBroker));
                    std::string clientIp(s_recv(m_tkBroker));
                    std::cout<<"taskInfo :"<<taskInfo<<"client ip: "<<clientIp<<std::endl;
//                    if(taskInfo != "SKREQ")
//                    {
//                       //当前版本的task info表示任务的创建时间，后续改进可以添加其他参数，如算力、网络资源
                        //这一步设计
                        m_tkPool.submit(std::bind(&LBBroker::TkGetHandle, this, std::placeholders::_1, std::placeholders::_2), taskInfo, clientIp);
//                    }
//                    else{
//                        //挑选合适的worker
//                        //这一部分容易阻塞住！！后面要注意
//                        std::string workerId(s_recv(m_tkBroker));
//                        std::cout<<"workerId :"<<workerId<<" Id length :"<<workerId.length()<<std::endl;
//                        //这一步没发到 worker那里
//                        int rc = zmq_send(m_hbBroker, workerId.c_str(), workerId.length(), ZMQ_SNDMORE);
//                        //int rc = zmq_send(m_hbBroker, "CC66-C879", 10, ZMQ_SNDMORE);
//                        assert(rc != -1);
//                        //zmq_send(m_hbBroker, "", 0, ZMQ_SNDMORE);
//                        zmq_send(m_hbBroker, "SKREQ", 5, ZMQ_SNDMORE);
//                        //zmq_send(m_hbBroker, "", 0, ZMQ_SNDMORE);
//                        zmq_send(m_hbBroker, clientId.c_str(), clientId.length(), 0);
//                        
//                        std::string clientId (s_recv(m_hbBroker));
//
//                        {
//                            // Second frame is empty
//                            std::string empty = s_recv(m_hbBroker);
//                            assert(empty.size() == 0);
//                        }
//
//                        // Third frame is READY or else a client reply address
//                        std::string workerIp(s_recv(m_hbBroker));
//
//                        zmq_send(m_tkBroker, clientId.c_str(), clientId.length(), ZMQ_SNDMORE);
//                        zmq_send(m_tkBroker, "", 0, ZMQ_SNDMORE);
//                        zmq_send(m_tkBroker, workerIp.c_str(), workerIp.length(), 0);
                        

                        //接收worker ip并返回
//                        std::string workerIp(s_recv(m_hbBroker));
//                        //105行指定client的逻辑应该交由worker实现，broker只负责进行转交并且这部分不能使用多线程。因为共同使用一个broker ip，如果使用多线程会导致数据错乱，进而导致一部分数据错误一部分线程阻塞
//                        zmq_send(m_tkBroker, clientId.c_str(), clientId.length(), ZMQ_SNDMORE);
//                        zmq_send(m_tkBroker, "", 0, ZMQ_SNDMORE);
//                        zmq_send(m_tkBroker, workerIp.c_str(), workerIp.length(), 0);

                        //补充单纯的转发逻辑,转发可单独作为线程实例,不过考虑一个问题。既然这部分采用了多线程，而router本身是多阶段通信的封装协议，那么如果同时有多个worker发起转发请求的话，broker本身应该就不能正常运作了。因为ZeroMQ的socket并非线程安全。所以这部分要再仔细考虑!
                        //socket发送应该是线程安全的，但是socket接收并不是线程安全的！
                        //看一下broker本身连续接收多条信息是否是线程安全的，记得样例代码每次worker只传送一条信息所以不涉及数据错乱问题。
//                    }

                }
            }
            std::cout<<"结束 "<<std::endl;
        }


        /*
         * Handle中包含了服务注册，以及线程分配。
         * */
        void HbGetHandle(std::string workerHbId){

            /*
             * redis存储信息,如果存在则更新键的过期时间，如果不存在则添加键值对。
             * 因为broker在与worker动态建立tk链接时，需要broker向worker的hb socket发送信息，因此保存的内容为 hb_identity, tk_identity, 插入时间
             *
             * 但是broker后续创建的tk链接可以是一对一的REQ-REP,因此可以去掉tk_identity, 保存格式改为 hb_identity, 插入时间
             *
             * 因为后续扩展会需要对设备的ability进行排序，为了扩展性考虑虽说设置了过期时间，但仍然要对插入时间进行排序
             * 后续不仅要维护过期时间，也要维护插入时间。这部分考虑不当，redis中是对键设置过期时间，而目前采用的是有序集合键结构，
             * 所以需要多键中的单个值设置过期时间。方法就是通过插入时间进行维护，通过zremrangebyscore set1 0 1522598920删除过期的值。
             * 目前考虑着是在挑选worker的时候，删除过期键
             * */

            redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
            if(context->err) {
                redisFree(context);
                printf("connect redisServer err:%s\n", context->errstr);
            }

            std::string cmdPre = "ZADD Worker "; //当值已经存在时会直接更新数值

            uint64_t insertTime = clock_time();

            std::string cmd = cmdPre + std::to_string(insertTime) + " " + workerHbId;
            redisReply *rdreply = (redisReply *)redisCommand(context, cmd.c_str());


            freeReplyObject(rdreply);
            printf("%s execute success\n", cmd.c_str());
            redisFree(context);
            zmq_send(m_hbBroker, workerHbId.c_str(), workerHbId.length(), ZMQ_SNDMORE);
            //zmq_send(m_hbBroker, "", 0, ZMQ_SNDMORE);
            zmq_send(m_hbBroker, "HB_OK", 5, 0);

        }

        void TkGetHandle(std::string taskInfo, std::string clientIp){
            //未添加切片逻辑

            //client端任务数据到达，根据redis中Worker键挑选合适的worker。首先要先处理过期键，再挑选新键作为目标worker。
            //挑选完后进行下一步的同步：在client与目的worker之间创建一对一的REP/REQ，后期的任务传输和结果返回就不需要再通过broker了
            //优点 ： 1.降低broker的带宽、计算压力 2.之前的同步代码可以直接使用了!

            //超过10s未更新的键认定为过期键
            
            uint64_t nowTime = clock_time();
            uint64_t deletTime = nowTime - 10000;
            
            redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
            if(context->err) {
                redisFree(context);
                printf("connect redisServer err:%s\n", context->errstr);
            }
            std::string cmd_1 = "zremrangebyscore Worker 0 ";
            cmd_1 += std::to_string(deletTime);

            //redisReply *rdreply = (redisReply *)redisCommand(context, cmd_1.c_str());
            //获取分数最大的值
            //std::string cmd_2 = "ZREVRANGEBYSCORE Worker +inf -inf WITHSCORES LIMIT 0 1";
            std::string cmd_2 = "zrange Worker -1 -1";
            redisReply *rdreply = (redisReply *)redisCommand(context, cmd_2.c_str());
            if(rdreply->type != REDIS_REPLY_ARRAY)
            {
                printf("command execute failure:\n");
                freeReplyObject(rdreply);
                redisFree(context);
                return ;
            }
            
            //缺少redis值更新代码，使用了该id的worker需要及时反馈回redis。不过每次worker的心跳就能够更新了。（高并发可以提感知的频率）
            std::string workerId((**rdreply->element).str);
            std::cout<<"Target worker id : "<<workerId<<std::endl;

            freeReplyObject(rdreply);
            printf("%s execute success\n", cmd_2.c_str());
            redisFree(context);

            //接下来就是将信息返回client中,需要worker获取一个随机空闲端口并与client进行同步,后续的同步就放在client的代码逻辑里了！
//            zmq_send(m_tkBroker, clientId.c_str(), clientId.length(), ZMQ_SNDMORE);
//            zmq_send(m_tkBroker, "", 0, ZMQ_SNDMORE);
//            zmq_send(m_tkBroker, worker.c_str(), worker.length(), 0);
            //zmq_send(m_hbBroker, workerId.c_str(), workerId.length(), ZMQ_SNDMORE);
            zmq_send(m_hbBroker, workerId.c_str(), strlen(workerId.c_str()), ZMQ_SNDMORE);
            //zmq_send(m_hbBroker, "SKREQ", 5, ZMQ_SNDMORE);

//            zmq_send(m_tkBroker, "", 0, ZMQ_SNDMORE);
            zmq_send(m_hbBroker, clientIp.c_str(), clientIp.length(), 0);

        }
        
        ~LBBroker(){
            m_hbPool.shutdown();
            m_tkPool.shutdown();
            zmq_close(m_hbBroker);
            zmq_close(m_tkBroker);
            zmq_ctx_destroy(m_context);
        }

    private:

        ThreadPool m_hbPool; //虽说hb线程中只涉及单message的往返通信，不会涉及太多的IO。但是会有对metric table的更新操作。而更新操作可能会消耗较多时间，所以此处也采用线程池处理。因为redis本身是单线程+IO多路服用，所以redis本身是线程安全的，不需要进行多余的同步控制
        ThreadPool m_tkPool; //采用reactor模式处理
        void *m_context;
        void *m_hbBroker; //heartbeat
        void *m_tkBroker; //task
        


};
#endif
