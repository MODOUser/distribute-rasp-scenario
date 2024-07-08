#include"./../../src/ftp/file-transfer.h"
using namespace std;
int main(int argc,char** argv){
    
    void * context = zmq_ctx_new();
    void * socket  = zmq_socket(context,ZMQ_REP);
    assert(socket);
    int rc = zmq_bind(socket,("tcp://*:6500"));
    assert(rc != -1);
    string filePath = "./model.h5";
    ReceiveFile(socket,filePath);
    zmq_ctx_destroy(context);
    return 0;

}
