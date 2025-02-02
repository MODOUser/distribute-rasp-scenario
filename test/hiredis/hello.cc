#include <stdio.h>
#include <string.h>
#include <hiredis/hiredis.h>

void test(void) 
{
    redisContext *context = redisConnect("127.0.0.1", 6379);//默认端口，本机redis-server服务开启
    if(context->err) {
        redisFree(context); 
        printf("connect redisServer err:%s\n", context->errstr);
        return ;
    } 

    printf("connect redisServer success\n"); 

//    const char *cmd = "SET test 100";
//    redisReply *reply = (redisReply *)redisCommand(context, cmd);
//
//    if(NULL == reply) {
//        printf("command execute failure\n");
//        redisFree(context);
//        return ;
//    }
//    //返回执行结果为状态的命令。比如set命令的返回值的类型是REDIS_REPLY_STATUS，然后只有当返回信息是"OK"时，才表示该命令执行成功。可以通过reply->str得到文字信息
//    if(!(reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0)) {
//        printf("command execute failure:%s\n", cmd); 
//        freeReplyObject(reply); 
//        redisFree(context);
//        return ;
//    }
//
//    freeReplyObject(reply);
//    printf("%s execute success\n", cmd);

    const char *getVal = "GET test";
    redisReply *reply = (redisReply *)redisCommand(context, getVal);

    if(reply->type != REDIS_REPLY_STRING)
    {
        printf("command execute failure:%s\n", getVal); 
        freeReplyObject(reply); 
        redisFree(context);
        return ;
    }

    printf("GET test:%s\n", reply->str);

    freeReplyObject(reply);
    redisFree(context);
}

int main(void)
{
    test();
    return 0;;
}
