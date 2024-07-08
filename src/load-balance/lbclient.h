#ifndef LBCLIENT_H
#define LBCLIENT_H
#include"lbhelper.h"
#include"./../distribute/synchronous.h"
#include<unistd.h>

/*
 * client ：先向broker发送任务的元信息。broker会返回一个workerId，然后client需要与相应worker建立REQ/REP的一对一链路进行任务传输。（设计思路写在lbbroker.h）
 * client与worker建立链接 ：
 * 1. client向固定worker发送链接建立请求
 * 2. worker收到请求后立马产生一个心跳包更新当前状态。并获取一个随机空闲端口，建立链接。并将端口及ip地址发送回client
 * 3. client接收到worker的地址信息后创建链接
 * 4. client发送任务并等待结果返回
 *
 * */

/*
 * client很有可能会遇到任务执行过程中就离线了，这部分需要一个反馈。目前暂时不考虑这种情况，client内部也不采用心跳检测。
 * client目前只支持一个client发送一个任务请求，后续可以添加多线程的请求支持。
 * */

class LBClient{
    public:
        //要提供的ip 例如 "tcp://localhost"
        LBClient(std::string sockip){
            m_context = zmq_ctx_new();
            m_client = zmq_socket(m_context, ZMQ_REQ);
            s_set_id(m_client);
            std::string tkip = sockip + std::string(":6001");
            //zmq_setsockopt(m_client, ZMQ_LINGER, 0, sizeof(0));
            int rc = zmq_connect(m_client, tkip.c_str());
            assert(rc == 0);
        }

        ~ LBClient(){
            zmq_close(m_client);
            zmq_ctx_destroy(m_context);
        }
        
        void ToBroker(){
            //发送任务的元信息
            //获取ip并创建socket
            void *synClient = zmq_socket(m_context, ZMQ_REQ);
            CreateSocket(synClient);
            //CreateSocket() 同时负责发送任务元信息跟ip地址。这部分职能分配不合理，等框架能跑通再返回来改吧
            
//            std::string workerId(s_recv(m_client));
//            std::cout<<"Ideal worker : "<<workerId<<std::endl;
//
//            //动态建立socket链接,因为broker只与worker的heart beat socket相连接。所以worker需要对hb_socket
//            //返回的信息进行甄别。 一种是常规的心跳返回，另一种是client的链接申请。
//            //心跳包发送：HB_OK
//            //client发送：SKREQ
//            zmq_send(m_client, "SKREQ", 5, ZMQ_SNDMORE);
//            zmq_send(m_client, workerId.c_str(), workerId.length(), 0);
//            std::string workerIp(s_recv(m_client));//该ip格式为"tcp://10.2.1.9:5656"
//            //创建新链接
//            void *synClient = zmq_socket(m_context, ZMQ_REQ);
//
//            int rc = zmq_connect(synClient, workerIp.c_str());
//            std::cout<<workerIp<<std::endl;
//            assert(rc == 0);
//            std::cout<<"链接创建完成 ，开始发送数据 "<<std::endl;
            ClientSyn(synClient);
            zmq_close(synClient);
            //任务结果直接存放到redis里面了，后续的操作这里省略了。直接cout结果

        }
        /*
         * Client发送前首先需要用户传入模型和数据文件。
         * 该方法负责接收用户的文件并保存至model/文件夹下。
         * 过程中会为任务生成一个唯一的id
         * */
//        int CliGetFile(std::string modelFile, std::string dataFile){
//            /*
//             * 生成唯一id
//             * */
//            /*
//             * 计算任务优先级
//             * */
//            //文件拷贝
//            std::string taskId = "13666";
//            std::string prefix = "./../../model/" ;
//            double priority = 0;
//            int isCreate = mkdir((prefix + taskId).c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
//            int modelInd = 0;
//            int dataInd = 0;
//            for(int i = 0 ; i < modelFile.length(); ++i){
//                if(modelFile[i] == '/'){
//                    modelInd = i + 1;
//                }
//            }
//            std::string model(modelFile.begin() + modelInd,modelFile.end());
//            for(int i = 0 ; i < dataFile.length(); ++i){
//                if(dataFile[i] == '/'){
//                    dataInd = i + 1;
//                }
//            }
//            std::string data(dataFile.begin() + dataInd,dataFile.end());
//            std::cout<<model<<'\t'<<data<<std::endl;
//            if(!LocalCopy(modelFile,prefix + taskId + '/' + model)){
//                std::cout<<"模型拷贝错误 "<<std::endl;
//                return -1;
//            }
//            if(!LocalCopy(dataFile,prefix + taskId + '/' + data)){
//                std::cout<<"数据拷贝错误 "<<std::endl;
//                return -1;
//            }
//            /*
//             * redis信息维护
//             * */
//            redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
//            if(context->err) {
//                redisFree(context);
//                printf("connect redisServer err:%s\n", context->errstr);
//                return -1;
//            }
//
//            printf("connect redisServer success\n");
//            std::string cmdPre = "ZADD taskid ";
//
//            std::string cmd = cmdPre + std::to_string(priority) + " " + taskId;
//            redisReply *reply = (redisReply *)redisCommand(context, cmd.c_str());
//
//            freeReplyObject(reply);
//            printf("%s execute success\n", cmd.c_str());
//            redisFree(context);
//            return 0;
//
//        }



    private:
        /*
         * Client负责发送文件
         * Server接收并执行文件
         * 这里传入的socket是动态创建的socket
         * */
//        int ClientSyn(void* socket){
//            /*
//             * 从redis里获取当前最需要执行的任务id. 有序集合
//             * */
//            redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
//            if(context->err) {
//                redisFree(context);
//                printf("connect redisServer err:%s\n", context->errstr);
//                return -1;
//            }
//            const char *getVal = "ZRANGE taskid -1 -1";
//            auto reply = (redisReply *)redisCommand(context, getVal);
//
//            std::string taskId = "";
//            if (reply->type == REDIS_REPLY_ARRAY) {
//                //taskId.assign(reply->element[0]->str);
//                taskId = reply->element[0]->str;
//            }
//            std::cout<<taskId<<std::endl;
//            freeReplyObject(reply);
//            redisFree(context);
//
//            std::string dataFile = "";
//            std::string modelFile = "";
//            std::string prefix("./../../model/"); 
//            long long int dataSize = 0;
//            long long int modelSize = 0;
//            //获取任务id。根据任务id取得目的文件夹（文件组织：模型---.h5  数据---.npy）
//            struct dirent *ptr;    
//            DIR *dir;
//            dir=opendir((prefix+taskId).c_str());
//            printf("文件列表: \n");
//            while((ptr=readdir(dir))!=NULL)
//            {
//                std::string file = ptr->d_name;
//                //跳过'.'和'..'两个目录
//                if(file[0] == '.')
//                    continue;
//                //检查后缀
//                std::cout<<file<<std::endl;
//                if(file.substr(file.length() - 2,2) == "h5"){
//                    modelFile = file;
//                }
//                if(file.substr(file.length() - 3,3) == "npy"){
//                    dataFile = file;
//                }
//            }
//            closedir(dir);
////            std::cout<<"等待 Hello"<<std::endl;
////            std::string trigger(s_recv(socket));
////            std::cout<<"From worker : "<<trigger<<std::endl;
//            zmq_send(socket, taskId.c_str(), taskId.length(), 0);
////            StringSend(socket,taskId.c_str());
//
//            zmq_msg_t receive;
//            zmq_msg_init(&receive);
//            zmq_msg_recv(&receive,socket,0);
//            std::cout<<(char*)zmq_msg_data(&receive)<<std::endl;
//
//            std::cout<<dataFile<<'\t'<<modelFile<<std::endl;
//            int dataSizeT = SendFile(socket,prefix+taskId+'/'+dataFile);
//            std::cout<<"原文件大小 ： "<<dataSize<<"  接收文件大小： "<<dataSizeT<<std::endl;
//            //    if(dataSizeT != dataSize){
//            //        std::cout<<"数据发送失败：data数据丢失"<<std::endl;
//            //        return -1;
//            //    }
//            int modelSizeT = SendFile(socket,prefix+taskId+'/'+modelFile);
//            std::cout<<"原文件大小 ： "<<modelSize<<"  接收文件大小： "<<modelSizeT<<std::endl;
//            //    if(modelSizeT != modelSize){
//            //        
//            //        std::cout<<"数据发送失败：model数据丢失"<<std::endl;
//            //        return -1;
//            //    }
//
//            //接收结果并将结果保存至redis中，接收到的信息只包含任务id和任务结果，结果用字符串存储吧。
//            std::string res(s_recv(socket));
////            redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
////            if(context->err) {
////                redisFree(context);
////                printf("connect redisServer err:%s\n", context->errstr);
////                return -1;
////            }
//            getVal = "ZRANGE taskid -1 -1";
//            reply = (redisReply *)redisCommand(context, "SELECT 1"); //将结果全部存放至1号数据库，与其他数据区分开
//            std::string cmd = "SET " + taskId + " " + res;
//            reply = (redisReply *)redisCommand(context, cmd.c_str()); //将结果全部存放至1号数据库，与其他数据区分开
//            std::cout<<res<<std::endl;
//            freeReplyObject(reply);
//            redisFree(context);
//
//            return 0;
//        }

        
        void CreateSocket(void *socket){
            //随机获取一个空闲port,动态port范围是1024～5000。
            //将ip发送给broker,指定具体worker的逻辑在broker里面实现了
            
            //获取port的逻辑：从1024到5000依次调用bind，如果返回值为-1，则表明当前接口非空闲，否则绑定成功。
            int port = 0;
            std::string ipPre("tcp://*:");
            for(int i = 10000; i <= 15000; ++i){
                string ip = ipPre + std::to_string(port);
                //int rc = zmq_bind(socket, ip.c_str());
                int rc = zmq_bind(socket, ("tcp://*:10000"));
                if(rc == 0){
                    port = i;
                    break;
                }
            }
            if(port == 0){
                std::cout<<"No available port !"<<std::endl;
                return;
            }
            //获取本地ip信息，将ip与port拼接后通过broker发送到client端。
//            //将任务信息和ip信息发送到broker
//            int rc = zmq_bind(socket, ("tcp://*:10000"));
            std::string ip = get_ip();
            std::string newIp = "tcp://" + ip + ":" + to_string(port);
            
//            zmq_send(m_hbworker, clientId.c_str(), clientId.length(), ZMQ_SNDMORE);
//            zmq_send(m_hbworker, "", 0, ZMQ_SNDMORE);
//            zmq_send(m_hbworker, workerIp.c_str(), workerIp.length(),0);
//            ServerSyn(newSock);
//            zmq_close(newSock);
            
            std::cout<<"New ip : "<<newIp<<std::endl;
            std::string startTime = std::to_string(clock_time());
            zmq_send(m_client, startTime.c_str(), startTime.length(), ZMQ_SNDMORE);
            zmq_send(m_client, newIp.c_str(), newIp.length(), 0);

        }
        void *m_context;
        void *m_client;
        int m_timeout;
        int m_retries;
};
#endif
