#include <cstring>
#include <string>
#include <hiredis/hiredis.h>
using namespace std;

void test(string taskid, double priority) 
{
    redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
    if(context->err) {
        redisFree(context); 
        printf("connect redisServer err:%s\n", context->errstr);
        return ;
    } 

    printf("connect redisServer success\n"); 
    string cmdPre = "ZADD taskid ";

    string cmd = cmdPre + taskid + " " + to_string(priority);  
    redisReply *reply = (redisReply *)redisCommand(context, cmd.c_str());

//    if(NULL == reply) {
//        printf("command execute failure\n");
//        redisFree(context);
//        return ;
//    }
    //返回执行结果为状态的命令。比如set命令的返回值的类型是REDIS_REPLY_STATUS，然后只有当返回信息是"OK"时，才表示该命令执行成功。可以通过reply->str得到文字信息
//    if(!(reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0)) {
//        printf("command execute failure:%s\n", cmd); 
//        freeReplyObject(reply); 
//        redisFree(context);
//        return ;
//    }

    freeReplyObject(reply);
    printf("%s execute success\n", cmd.c_str());

    const char *getVal = "ZRANGE taskid -1 -1";
    reply = (redisReply *)redisCommand(context, getVal);
    if (reply->type == REDIS_REPLY_ARRAY) {
//        for (unsigned int j = 0; j < reply->elements; j++) {
//            printf("%u) %s\n", j, reply->element[j]->str);
//        }
          printf("%u) %s\n", 0, reply->element[0]->str);
          
    }

//    if(reply->type != REDIS_REPLY_STRING)
//    {
//        printf("command execute failure:%s\n", getVal); 
//        freeReplyObject(reply); 
//        redisFree(context);
//        return ;
//    }


    freeReplyObject(reply);
    redisFree(context);
}
