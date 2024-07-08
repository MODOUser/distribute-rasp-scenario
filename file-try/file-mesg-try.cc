#include<zmq.h>
#include<unistd.h>
#include<stdio.h>
#include<iostream>
#include<cassert>
#include<cstring>
#include<fstream>
/*
 * client 负责发送文件到server并接收server的返回信息，将返回信息写入文件
 * server 负责接收client发送的文件，并产生结果将结果返回client
 *
 */


/*
 * 需要注意的问题：文件读写以及中间的char[]变量的长度要一致，需要定义
 * 一个结构体，来对每个小数据的大小进行规范
 */

//#define BUFFSIZE 5*1024*1024
//std::string FileRead(const char *filePath){
//    FILE* fd = ::fopen(filePath,"rb");
//    std::string content;
//    if(fd){
//        char buf[BUFFSIZE];
//        while(fread(buf,1,sizeof(buf),fd) > 0){
//            content.append(buf,BUFFSIZE);
//        }
//        ::fclose(fd);
//    }
//
//    return content;
//}
//void FileWrite(const char *filePath,const char *content,int size){
//    FILE* fd = ::fopen(filePath,"ab");
//    if(fd){
//        fwrite(content,size,1,fd);
//    }
//}
#define BUFFSIZE 1024
void WriteFile(char* content, string filePath,int size){
    ofstream outFile(filePath, ios::app | ios::binary);
    outFile.write(content, size);
    outFile.close();
}
void ReadFile(const string &filePath){
    ifstream inFile(filePath, ios::in | ios::binary);
    if (!inFile) {
        cout << "error" << endl;
        return ;
    }
    char c[BUFFSIZE];
    //连续以行为单位，读取 in.txt 文件中的数据
//    while (inFile.getline(c,BUFFSIZE)) {
//        WriteFile(c,"./receved-file");
//    }
    while(inFile.read(c,BUFFSIZE)){
        WriteFile(c,"./receved-file.h5",BUFFSIZE);
    }  
    int size = inFile.gcount();
    WriteFile(c,"./receved-file.h5",size);
    inFile.close();
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
    zmq_msg_t message;
    zmq_msg_init_size(&message,strlen(file.c_str()));
    memcpy(zmq_msg_data(&message),file.c_str(),strlen(file.c_str()));
    //rc = zmq_msg_init_data(&message,&file,file.length(),NULL,NULL);
    assert(rc != -1);
    std::cout<<"Start sending file. "<<file.length()<<std::endl;

    //zmq_send(socketSend,"hellofile",10,0);
    rc = zmq_msg_send(&message,socketSend,0);
    //zmq_send(socketSend,"hello",6,0);
    
    zmq_msg_t receive;
    zmq_msg_init(&receive);
    zmq_msg_recv(&receive,socketSend,0);
//    char receive[3];
//    zmq_recv(socketSend,receive,2,0);
//    std::cout<<receive<<std::endl;
    zmq_close(socketSend);
}
static void serverRoutine(void *context,const char* filePath){
    void * socket  = zmq_socket(context,ZMQ_REP);
    assert(socket);
    int rc = zmq_bind(socket,"tcp://*:6555");
    assert(rc != -1);
//    zmq_msg_t receive;
//    zmq_msg_init(&receive);
//    std::cout<<"Start receiving."<<std::endl;
//    int size = zmq_msg_recv(&receive,socket,0);

    zmq_msg_t msg;
    rc = zmq_msg_init (&msg);
    assert (rc == 0);
    /* Block until a message is available to be received from socket */
    rc = zmq_msg_recv (&msg, socket, 0);
    assert (rc != -1);
    /* Release message */
    //zmq_msg_close (&msg);

    char temp[rc + 1];
    memcpy(&temp,zmq_msg_data(&msg),rc);
    //temp[rc] = '\0';
    std::cout<<rc<<std::endl;
//    std::cout<<"File received "<<temp<<std::endl;
    FileWrite(filePath,temp,rc);
    //assert(rc);
    zmq_send(socket,"OK",2,0);
    zmq_close(socket);
}

int main(int argc,char** argv){
    
    void * context = zmq_ctx_new();
    int i = std::stoi(argv[1]);
    if(i == 1){
        std::cout<<"c";
//        while(1){
//            clientRoutine(context,"./test");
//        }
        clientRoutine(context,"./../model_json");
    }
    else{
        std::cout<<"s";
//        while(true){
//            serverRoutine(context,"./receved-file");
//        }
        serverRoutine(context,"./receved-file");
    }
    zmq_ctx_destroy(context);

//    std::string file = FileRead("./testdata");
//    FileWrite("./receved-file",file.c_str());
    return 0;

}
