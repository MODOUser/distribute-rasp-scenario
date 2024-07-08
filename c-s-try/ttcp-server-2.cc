#include<zmq.h>
#include<string>
#include<cassert>
#include<unistd.h>
#include<iostream>
int main(){

    void* context = zmq_ctx_new();
    void* socket = zmq_socket(context,ZMQ_REP);
    
    assert(socket);
    int rc = zmq_bind(socket,"tcp://*:5555");
    assert(rc != -1);

    int pktNums = 50;
    int pktSize = 20;

    for(int i = 0; i != pktNums; ++i){
        
//        std::string receive;
        char receive[20];

//        if(i < pktNums - 1){
//            rc = zmq_recv(socket,&receive,pktSize,ZMQ_RCVMORE);
//        }
//        else{
//            rc = zmq_recv(socket,&receive,pktSize,0);
//        }
        rc = zmq_recv(socket,&receive,pktSize+1,0);
        std::cout<<"Successful receive "<<i<<"st paket."<<std::endl; 
        assert(rc == pktSize);
        sleep(1);

        zmq_send(socket,"OK",2,0);
        
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);
    
}
