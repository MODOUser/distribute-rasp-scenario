#ifndef FileTransfer_H
#define FileTransfer_H
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
class FileTransferClient{
    public: 
        FileTransferClient(string ipPostfix):m_ipPostfix(ipPostfix){m_ip = m_ipPrefix + m_ipPostfix;}
        void ClientRoutine(const char* filePath);
        void ReceiveFile(char* content, string filePath,int size);
    private:
        void Initial();
        void* m_socket;
        void* m_context;
        string m_ipPrefix = "tcp://";
        string m_ipPostfix ;
        string m_ip ;
        
};
class FileTransferServer{
    public: 
        FileTransferServer(string ipPostfix):m_ipPostfix(ipPostfix){m_ip = m_ipPrefix + m_ipPostfix;}
        void ServerRoutine(const char* filePath);
        void SendFile(const string &filePath);
    private:
        void Initial();
        void* m_socket;
        void* m_context;
        string m_ipPrefix = "tcp://";
        string m_ipPostfix = "";
        string m_ip = "";
        
};
#endif
