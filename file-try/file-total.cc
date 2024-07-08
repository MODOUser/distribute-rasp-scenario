#include<zmq.h>
#include<unistd.h>
#include<stdio.h>
#include<iostream>
#include<cassert>
/*
 * client 负责发送文件到server并接收server的返回信息，将返回信息写入文件
 * server 负责接收client发送的文件，并产生结果将结果返回client
 *
 */


/*
 * 需要注意的问题：文件读写以及中间的char[]变量的长度要一致，需要定义
 * 一个结构体，来对每个小数据的大小进行规范
 */
std::string FileRead(const char *filePath){
    FILE* fd = ::fopen(filePath,"r");
    std::string content;
    if(fd){
        int buffsize = 20;
        char buf[buffsize];
        int size = 0;
        while((size = fread(buf,1,sizeof(buf),fd)) > 0){
            content.append(buf,size);
        }
        ::fclose(fd);
    }

    return content;
}
void FileWrite(const char *filePath,const char *content){
    FILE* fd = ::fopen(filePath,"w");
    if(fd){
        fwrite(content,1,10,fd);
    }
}
//string FileWrite(const char &filePath,string content){
//    
//}
static void clientRoutine(void* context,const char* filePath){
    void * socketSend = zmq_socket(context, ZMQ_REQ);
    assert(socketSend);
    int rc = zmq_connect(socketSend,"tcp://localhost:6555");
    assert(rc != -1);
    std::cout<<"Start reading file. "<<std::endl;
    //void * socketListen = zmq_socket(context, ZMQ_REP);
    std::string file = FileRead(filePath);
    std::cout<<"Start sending file. "<<std::endl;

    zmq_send(socketSend,"hellofile",10,0);
    char receive[3];
    zmq_recv(socketSend,receive,2,0);
    std::cout<<receive<<std::endl;
    zmq_close(socketSend);
}
static void serverRoutine(void *context,const char* filePath){
    void * socket  = zmq_socket(context,ZMQ_REP);
    assert(socket);
    int rc = zmq_bind(socket,"tcp://*:6555");
    assert(rc != -1);
    char receive[21];
    std::cout<<"Start receiving."<<std::endl;
    rc = zmq_recv(socket,receive,10,0);
    std::string temp(receive);
    std::cout<<"File received "<<temp<<std::endl;
    FileWrite(filePath,receive);
    assert(rc);
    zmq_send(socket,"OK",2,0);
    zmq_close(socket);
}

int main(int argc,char** argv){
    
    void * context = zmq_ctx_new();
    //int i = std::stoi(argv[1]);
    if(i == 1){
        std::cout<<"c";
        clientRoutine(context,"./hello-file");
    }
    else{
        std::cout<<"s";
        serverRoutine(context,"./receved-file");
    }

    zmq_ctx_destroy(context);
    return 0;

}
