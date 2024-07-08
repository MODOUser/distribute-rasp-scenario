#include <zmq.h>
#include <cstring>
#include <iostream>
#include "../../src/ftp/file-transfer.h"
int main () { 
    std::cout<<"hhh";
    void *context = zmq_ctx_new ();
    void *sink = zmq_socket (context, ZMQ_ROUTER);
    zmq_bind (sink, "tcp://*:6500");
    // First allow 0MQ to set the identity 
    void *anonymous = zmq_socket (context, ZMQ_REQ);
    zmq_connect (anonymous, "tcp://localhost:6500");
//    zmq_msg_t msg;
//    zmq_msg_init_size(&msg, 5);
//    memcpy(zmq_msg_data(&msg),"HELLO",5);
    string te = "Hello";
    StringSend(anonymous,te.c_str());
    std::cout<<"send"<<std::endl;
    //zmq_msg_send(&msg,anonymous,0);

    zmq_msg_t receive1;
//    zmq_msg_recv(&receive1,sink,0);
//    std::cout<<(char *)zmq_msg_data(&receive1)<<std::endl;
    string id(StringRecv(sink));
    std::cout<<"receive  "<<id<<std::endl;

    // Then set the identity ourselves 
    void *identified = zmq_socket (context, ZMQ_REQ);
    zmq_setsockopt (identified, ZMQ_METADATA, "ID:666", 6);
    zmq_connect (identified, "inproc://example");
    zmq_msg_t msg2;
    zmq_msg_init_size(&msg2, 5);
    memcpy(zmq_msg_data(&msg2),"HELLO",5);
    zmq_msg_send(&msg2,anonymous,0);
    zmq_msg_t receive2;
    zmq_msg_recv(&receive2,sink,0);
//    const char* routing_id2 = zmq_msg_gets(&receive2,"ID");
//    std::cout<<routing_id2<<std::endl;
    zmq_close (sink);
    zmq_close (anonymous);
    zmq_close (identified);
    zmq_ctx_destroy (context);
    return 0;
}
