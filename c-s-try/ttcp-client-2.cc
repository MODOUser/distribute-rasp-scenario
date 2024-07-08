#include<zmq.h>
#include<iostream>
#include<string>
#include<cassert>
int main(){
    
    void* context = zmq_ctx_new();
    void* socket = zmq_socket(context,ZMQ_REQ);
    assert(socket);

    int rc = zmq_connect(socket,"tcp://localhost:5555");
    assert(rc == 0);

    int pktNums = 50;
    int pktSize = 20;

    std::cout<<"Start Sending "<<std::endl;
    for(int i = 1; i <= pktNums; ++i){
        std::string content(20,'*');
        if(i % 3 != 0){
            rc = zmq_send(socket,&content,pktSize,ZMQ_SNDMORE);
//            rc = zmq_send(socket,&content,pktSize,0);
            
            assert(rc == pktSize);
        }
        else{
            rc = zmq_send(socket,&context,pktSize,0);
            assert(rc == pktSize);
        }
        
//        std::string buffer;
        char buffer[3];
        zmq_recv(socket,&buffer,2,0);
        std::cout<<"Successful send "<<i<<"st packet."<<std::endl;
        std::string buf(buffer);
        std::cout<<buf<<'\t';
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);

}
