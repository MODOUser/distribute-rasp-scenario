#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H
#include<zmq.h>
#include<unistd.h>
#include<stdio.h>
#include<iostream>
#include<cassert>
#include<cstring>
#include<fstream>
/*
 * server 负责发送文件到server并接收server的返回信息，将返回信息写入文件
 * client 负责接收client发送的文件，并产生结果将结果返回client
 *
 */


/*
 * 需要注意的问题：文件读写以及中间的char[]变量的长度要一致，需要定义
 * 一个结构体，来对每个小数据的大小进行规范
 */
#define BUFFSIZE 1024
using namespace std;
void WriteFile(char* content, string filePath,int size){
    ofstream outFile(filePath, ios::app | ios::binary);
    outFile.write(content, size);
    outFile.close();
}
/*
 * filePath 为要发送的文件地址（相对地址）
 * 默认为block IO
 * */
int SendFile(void* socket, const string &filePath){
    ifstream inFile(filePath, ios::in | ios::binary);
    if (!inFile) {
        cout << "error" << endl;
        return 0;
    }
    long long int fileSize = 0;
    char c[BUFFSIZE];
    while(inFile.read(c,BUFFSIZE)){
        zmq_msg_t message;
        zmq_msg_init_size(&message,BUFFSIZE);
        memcpy(zmq_msg_data(&message),c,BUFFSIZE);
        std::cout<<"Start sending file. "<<BUFFSIZE<<std::endl;

        int rc = zmq_msg_send(&message,socket,0);
        fileSize += rc;
        assert(rc != -1);

        zmq_msg_t receive;
        zmq_msg_init(&receive);
        zmq_msg_recv(&receive,socket,0);
    }  
    int size = inFile.gcount();
    zmq_msg_t message;
    zmq_msg_init_size(&message,size);
    memcpy(zmq_msg_data(&message),c,size);
    std::cout<<"Start sending file. "<<size<<std::endl;

    int rc = zmq_msg_send(&message,socket,0);
    fileSize += rc;
    assert(rc != -1);

    zmq_msg_t receive;
    zmq_msg_init(&receive);
    zmq_msg_recv(&receive,socket,0);
    cout<<"Sending complete. "<<endl;
    inFile.close();
    return fileSize;
}
/*
 * filePath 为要存放的地址
 * */
int ReceiveFile(void* socket, const string filePath){
    long long int fileSize = 0;
    zmq_msg_t msg;
    int rc = zmq_msg_init (&msg);
    assert (rc == 0);
    /* Block until a message is available to be received from socket */
    while(rc = zmq_msg_recv(&msg,socket,0)){

        fileSize += rc;
        char temp[rc];
        memcpy(&temp,zmq_msg_data(&msg),rc);
        std::cout<<rc<<std::endl;
        WriteFile(temp,filePath,rc);
        zmq_send(socket,"OK",2,0);
        if(rc < 1024){
            break;
        }
    }
    cout<<"Receive complete. "<<endl;
    return fileSize;
}
// 从套接字接收ØMQ字符串，并将其转换为C/C++字符串(在尾部添加0)
static char * StringRecv(void* socket)
{
    // 此处使用zmq_msg_init()初始化即可, zmq_msg_recv()在内部会自动对zmq_msg_t对象进行大小设定
    zmq_msg_t message;
    zmq_msg_init(&message);

    int size = zmq_msg_recv(&message, socket, 0);
    if(size == -1)
        return NULL;

    char *s = (char*)malloc(size + 1);
    memcpy(s, zmq_msg_data(&message), size);

    zmq_msg_close(&message);
    s[size] = 0;
    //string temp(s);
    return s;
}
// 将C字符串转换为ØMQ字符串(去掉尾部的'\0')，并发送到指定的套接字上
static int StringSend(void *socket,const char *string)
{
    // 因为是将数据拷贝给zmq_msg_t对象, 因此需要使用zmq_msg_init_size进行初始化
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, strlen(string));
    memcpy(zmq_msg_data(&msg), string, strlen(string));
 
    // 发送数据
    int rc = zmq_msg_send(&msg, socket, 0);
 
    // 关闭zmq_msg_t对象
    zmq_msg_close(&msg);
 
    return rc;
}
//本地文件拷贝
bool LocalCopy(string src, string dest) {
    ifstream is(src, ios::binary);
    if (is.fail()) {
        return false;
    }

    ofstream os(dest, ios::binary);
    if (os.fail()) {
        return false;
    }

    is.seekg(0, ios::end);
    long long length = is.tellg();  // C++ 支持的最大索引位置
    is.seekg(0);
    char buf[2048];
    while (length > 0)
    {
        int bufSize = length >= 2048 ? 2048 : length;
        is.read(buf, bufSize);
        os.write(buf, bufSize);
        length -= bufSize;
    }

    is.close();
    os.close();
    return true;
}
#endif

