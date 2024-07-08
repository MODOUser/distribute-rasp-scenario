#include"./../../src/ftp/file-transfer.h"
using namespace std;
int main(int argc,char** argv){
    
    void * context = zmq_ctx_new();
    void * socketSend = zmq_socket(context, ZMQ_REQ);
    assert(socketSend);
    int rc = zmq_connect(socketSend,("tcp://localhost:6500"));
    assert(rc != -1);
    string filePath = "./../../model/4LayerModel.h5";
    SendFile(socketSend,filePath);
    zmq_close(socketSend);
    zmq_ctx_destroy(context);

    return 0;

}
