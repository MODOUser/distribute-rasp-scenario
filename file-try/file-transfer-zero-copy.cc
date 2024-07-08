#include<zmq.h>
#include<unistd.h>
#include<stdio.h>
#include<iostream>
#include<cassert>
#include<cstring>
#include<fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
/*
 * server 负责发送文件到server并接收server的返回信息，将返回信息写入文件
 * client 负责接收client发送的文件，并产生结果将结果返回client
 *
 */


/*
 * 需要注意的问题：文件读写以及中间的char[]变量的长度要一致，需要定义
 * 一个结构体，来对每个小数据的大小进行规范
 */

//使用mmap + zmq_init_data减少cpu拷贝次数
#define BUFFSIZE 1024
using namespace std;
void my_free (void *data, void *hint) { free (data); } // Send message from buffer, which we allocate and 0MQ will free for us 


void WriteFile(char* content, string filePath,int size){
    ofstream outFile(filePath, ios::app | ios::binary);
    outFile.write(content, size);
    outFile.close();
}
void ReadFile(const string &filePath, void *socket){
//    ifstream inFile(filePath, ios::in | ios::binary);
//    if (!inFile) {
//        cout << "error" << endl;
//        return ;
//    }
//    char c[BUFFSIZE];
//    while(inFile.read(c,BUFFSIZE)){
//        zmq_msg_t message;
//        zmq_msg_init_size(&message,BUFFSIZE);
//        memcpy(zmq_msg_data(&message),c,BUFFSIZE);
//        std::cout<<"Start sending file. "<<BUFFSIZE<<std::endl;
//
//        int rc = zmq_msg_send(&message,socket,0);
//        assert(rc != -1);
//
//        zmq_msg_t receive;
//        zmq_msg_init(&receive);
//        zmq_msg_recv(&receive,socket,0);
//    }  
//    int size = inFile.gcount();
//    zmq_msg_t message;
//    zmq_msg_init_size(&message,size);
//    memcpy(zmq_msg_data(&message),c,size);
    int fd = open(filePath.c_str(), O_RDONLY);  
    // 读取文件长度
    int len = lseek(fd,0,SEEK_END);  
    // 建立内存映射
    char *addr = (char *) mmap(NULL, len, PROT_READ, MAP_PRIVATE,fd, 0);      
    close(fd);
    // data用于保存读取的数据
    char* data; 
    // 复制过来
    //std::cout<<"hhhhh"<<std::endl;
    //memcpy(data, addr, len);
    // 解除映射
    zmq_msg_t message; 
    zmq_msg_init_data (&message, addr, len, NULL, NULL);
    zmq_msg_send(&message,socket,0);
    std::cout<<"Start sending file. "<<len<<std::endl;
    munmap(addr, len);


    zmq_msg_t receive;
    zmq_msg_init(&receive);
    zmq_msg_recv(&receive,socket,0);
}
static void serverRoutine(void* context,const char* filePath, string addr){
    void * socketSend = zmq_socket(context, ZMQ_REQ);
    assert(socketSend);
    int rc = zmq_connect(socketSend,("tcp://" + addr).c_str());
    assert(rc != -1);
    std::cout<<"Start reading file. "<<std::endl;
    ReadFile(filePath,socketSend);
    zmq_close(socketSend);
}
static void clientRoutine(void *context,const char* filePath, string addr){
    void * socket  = zmq_socket(context,ZMQ_REP);
    assert(socket);
    int rc = zmq_bind(socket,("tcp://" + addr).c_str());
    assert(rc != -1);

    zmq_msg_t msg;
    rc = zmq_msg_init (&msg);
    assert (rc == 0);
    rc = zmq_msg_recv(&msg,socket,0);
    char temp[rc];
    memcpy(&temp,zmq_msg_data(&msg),rc);
    std::cout<<rc<<std::endl;
    WriteFile(temp,filePath,rc);
    /* Block until a message is available to be received from socket */
//    while(rc = zmq_msg_recv(&msg,socket,0)){
//
//        char temp[rc];
//        memcpy(&temp,zmq_msg_data(&msg),rc);
//        std::cout<<rc<<std::endl;
//        WriteFile(temp,filePath,rc);
//        zmq_send(socket,"OK",2,0);
//        if(rc < 1024){
//            break;
//        }
//    }
    zmq_close(socket);
}

int main(int argc,char** argv){
    
    void * context = zmq_ctx_new();
    int i = std::stoi(argv[1]);
    string addr= argv[2];
    string file = argv[3];
    if(i == 1){
        std::cout<<"s";
//        serverRoutine(context,"./../model.tflite",addr);
        serverRoutine(context,file.c_str(),addr);
    }
    else{
        std::cout<<"c";
        clientRoutine(context,file.c_str(),addr);
    }
    zmq_ctx_destroy(context);

    return 0;

}
