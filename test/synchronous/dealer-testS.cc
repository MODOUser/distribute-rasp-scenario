#include"../../src/distribute/synchronous.h"
using namespace std;
int main(int argc,char** argv){
    
    void * context = zmq_ctx_new();
    void * socket = zmq_socket(context, ZMQ_REP);
    assert(socket);
    int rc = zmq_connect(socket,("tcp://localhost:6500"));
    cout<<"hh";
    ServerSyn(socket);
    zmq_close(socket);
    zmq_ctx_destroy(context);

    return 0;

}
