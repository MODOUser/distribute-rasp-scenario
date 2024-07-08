#include"../../src/distribute/synchronous.h"
using namespace std;
int main(int argc,char** argv){
    
//    CliGetFile("./../../model/4LayerModeltail.h5","./../../model/head_output.npy");
    void * context = zmq_ctx_new();
    void * socket  = zmq_socket(context,ZMQ_REQ);
    assert(socket);
    int rc = zmq_bind(socket,("tcp://*:6500"));
    assert(rc != -1);
    ClientSyn(socket);
    zmq_close(socket);
    zmq_ctx_destroy(context);
    return 0;

}
