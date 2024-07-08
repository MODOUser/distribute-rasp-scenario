#ifndef SYNCHRONOUS_H
#define SYNCHRONOUS_H
#include<zmq.h>
#include<iostream>
#include"./../ftp/file-transfer.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <hiredis/hiredis.h>
using namespace std;
/*
 * Client发送前首先需要用户传入模型和数据文件。
 * 该方法负责接收用户的文件并保存至model/文件夹下。
 * 过程中会为任务生成一个唯一的id
 * */
int CliGetFile(string modelFile, string dataFile){
    /*
     * 生成唯一id
     * */
    /*
     * 计算任务优先级
     * */
    //文件拷贝
    string taskId = "13666";
    string prefix = "./../../model/" ;
    double priority = 0;
    int isCreate = mkdir((prefix + taskId).c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    int modelInd = 0;
    int dataInd = 0;
    for(int i = 0 ; i < modelFile.length(); ++i){
        if(modelFile[i] == '/'){
            modelInd = i + 1;
        }
    }
    string model(modelFile.begin() + modelInd,modelFile.end());
    for(int i = 0 ; i < dataFile.length(); ++i){
        if(dataFile[i] == '/'){
            dataInd = i + 1;
        }
    }
    string data(dataFile.begin() + dataInd,dataFile.end());
    cout<<model<<'\t'<<data<<endl;
    if(!LocalCopy(modelFile,prefix + taskId + '/' + model)){
        cout<<"模型拷贝错误 "<<endl;
        return -1;
    }
    if(!LocalCopy(dataFile,prefix + taskId + '/' + data)){
        cout<<"数据拷贝错误 "<<endl;
        return -1;
    }
    /*
     * redis信息维护
     * */
    redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
    if(context->err) {
        redisFree(context);
        printf("connect redisServer err:%s\n", context->errstr);
        return -1;
    }

    printf("connect redisServer success\n");
    string cmdPre = "ZADD taskid ";

    string cmd = cmdPre + to_string(priority) + " " + taskId;
    redisReply *reply = (redisReply *)redisCommand(context, cmd.c_str());

    freeReplyObject(reply);
    printf("%s execute success\n", cmd.c_str());
    redisFree(context);
    return 0;

}



/*
 * Client负责发送文件
 * Server接收并执行文件
 * */
int ClientSyn(void* socket){
    /*
     * 从redis里获取当前最需要执行的任务id. 有序集合
     * */
    redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
    if(context->err) {
        redisFree(context);
        printf("connect redisServer err:%s\n", context->errstr);
        return -1;
    }
    const char *getVal = "ZRANGE taskid -1 -1";
    auto reply = (redisReply *)redisCommand(context, getVal);

    string taskId = "";
    if (reply->type == REDIS_REPLY_ARRAY) {
          taskId.assign(reply->element[0]->str);
    }
    cout<<taskId<<'\t'<<"gggggg"<<endl;
    freeReplyObject(reply);
    redisFree(context);

    string dataFile = "";
    string modelFile = "";
    string prefix("./../../model/"); 
    long long int dataSize = 0;
    long long int modelSize = 0;
    //获取任务id。根据任务id取得目的文件夹（文件组织：模型---.h5  数据---.npy）
    struct dirent *ptr;    
    DIR *dir;
    dir=opendir((prefix+taskId).c_str());
    printf("文件列表: \n");
    while((ptr=readdir(dir))!=NULL)
    {
        string file = ptr->d_name;
        //跳过'.'和'..'两个目录
        if(file[0] == '.')
            continue;
        //检查后缀
        cout<<file<<endl;
        if(file.substr(file.length() - 2,2) == "h5"){
            modelFile = file;
        }
        if(file.substr(file.length() - 3,3) == "npy"){
            dataFile = file;
        }
    }
    closedir(dir);
    StringSend(socket,taskId.c_str());

    zmq_msg_t receive;
    zmq_msg_init(&receive);
    zmq_msg_recv(&receive,socket,0);
    cout<<(char*)zmq_msg_data(&receive)<<endl;

    cout<<dataFile<<'\t'<<modelFile<<endl;
    int dataSizeT = SendFile(socket,prefix+taskId+'/'+dataFile);
    cout<<"原文件大小 ： "<<dataSize<<"  接收文件大小： "<<dataSizeT<<endl;
//    if(dataSizeT != dataSize){
//        cout<<"数据发送失败：data数据丢失"<<endl;
//        return -1;
//    }
    int modelSizeT = SendFile(socket,prefix+taskId+'/'+modelFile);
        cout<<"原文件大小 ： "<<modelSize<<"  接收文件大小： "<<modelSizeT<<endl;
//    if(modelSizeT != modelSize){
//        
//        cout<<"数据发送失败：model数据丢失"<<endl;
//        return -1;
//    }
    return 0;
}
int ServerSyn(void* socket){


    string id(StringRecv(socket));
    string taskId =string("./../../model/re") + id;
    cout<<taskId<<endl;

    zmq_msg_t reply;
    zmq_msg_init_size (&reply, 7);
    cout<<"DATA_OK"<<endl;
    memcpy (zmq_msg_data (&reply), "DATA_OK", 7);
    zmq_msg_send (&reply, socket, 0);

    //文件接收后将任务摘要保存在数据库里,并且要与本机文件区分开.加一个前缀re区分
    
    //创建文件夹,保存数据
    int isCreate = mkdir(taskId.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    ReceiveFile(socket,(taskId + "/data.npy").c_str());
    ReceiveFile(socket,(taskId + "/model.h5").c_str());
    /*
     * 信息维护
     * */
    redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
    if(context->err) {
        redisFree(context);
        printf("connect redisServer err:%s\n", context->errstr);
        return -1;
    }

    string cmdPre = "LPUSH receive-task ";

    string cmd = cmdPre + id ;
    redisReply *rdreply = (redisReply *)redisCommand(context, cmd.c_str());

    freeReplyObject(rdreply);
    printf("%s execute success\n", cmd.c_str());
    redisFree(context);
    
    return 0;

    
}

#endif
