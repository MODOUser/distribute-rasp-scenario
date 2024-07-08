#include"file-transfer.h"
using namespace std;
/*-----------------------------file-transfer-server-----------------------------*/
void FileTransferServer::SendFile(const string &filePath){
    ifstream inFile(filePath, ios::in | ios::binary);
    if (!inFile) {
        cout << "error" << endl;
        return ;
    }
    char c[BUFFSIZE];
    while(inFile.read(c,BUFFSIZE)){
        zmq_msg_t message;
        zmq_msg_init_size(&message,BUFFSIZE);
        memcpy(zmq_msg_data(&message),c,BUFFSIZE);
        std::cout<<"Start sending file. "<<BUFFSIZE<<std::endl;

        int rc = zmq_msg_send(&message,m_socket,0);
        assert(rc != -1);

        zmq_msg_t receive;
        zmq_msg_init(&receive);
        zmq_msg_recv(&receive,m_socket,0);
    }  
    int size = inFile.gcount();
    zmq_msg_t message;
    zmq_msg_init_size(&message,size);
    memcpy(zmq_msg_data(&message),c,size);
    std::cout<<"Start sending file. "<<size<<std::endl;
    int rc = zmq_msg_send(&message,m_socket,0);
    assert(rc != -1);

    zmq_msg_t receive;
    zmq_msg_init(&receive);
    zmq_msg_recv(&receive,m_socket,0);
    inFile.close();
}
void FileTransferServer::Initial(){
    m_context = zmq_ctx_new();
    void * m_socket = zmq_socket(m_context, ZMQ_REQ);
    assert(m_socket);
    int rc = zmq_connect(m_socket,(m_ip).c_str());
    if(rc != -1){
        cout<<"ERROR : zmq_connect fail.";
        abort();
    }
}
void FileTransferServer::ServerRoutine(const char* filePath){
    std::cout<<"Start reading file. "<<std::endl;
    SendFile(filePath);
    zmq_close(m_socket);
    zmq_ctx_destroy(m_context);
}
/*-----------------------------file-transfer-client-----------------------------*/
void FileTransferClient::ReceiveFile(char* content, string filePath,int size){
    ofstream outFile(filePath, ios::app | ios::binary);
    outFile.write(content, size);
    outFile.close();
}
void FileTransferClient::Initial(){
    m_context = zmq_ctx_new();
    void * m_socket  = zmq_socket(m_context,ZMQ_REP);
    assert(m_socket);
    int rc = zmq_bind(m_socket,(m_ip).c_str());
    if(rc != -1){
        cout<<"ERROR : zmq_connect fail.";
        abort();
    }

}
void FileTransferClient::ClientRoutine(const char* filePath){
    zmq_msg_t msg;
    int rc = zmq_msg_init (&msg);
    assert (rc == 0);
    /* Block until a message is available to be received from socket */
    while(rc = zmq_msg_recv(&msg,m_socket,0)){
        char temp[rc];
        memcpy(&temp,zmq_msg_data(&msg),rc);
        std::cout<<rc<<std::endl;
        ReceiveFile(temp,filePath,rc);
        zmq_send(m_socket,"OK",2,0);
        if(rc < 1024){
            break;
        }
    }
    zmq_close(m_socket);
    zmq_ctx_destroy(m_context);
}
