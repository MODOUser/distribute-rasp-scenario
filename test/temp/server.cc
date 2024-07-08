#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <iostream>

int main (void)
{
    //  Socket to talk to clients
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_ROUTER);
    int rc = zmq_bind (responder, "tcp://*:5555");
    assert (rc == 0);

    while (1) {
        zmq_msg_t receive1;
        zmq_msg_init(&receive1);
        int rc = zmq_msg_recv(&receive1,responder,0);
        //zmq_recv (responder, buffer, 10, 0);
        printf ("Received Hello\n");
        //std::cout<<zmq_msg_routing_id(&receive1)<<std::endl;
        size_t size = zmq_msg_size(&receive1);
        std::string data(static_cast<char*>(zmq_msg_data(&receive1)), size);
        std::cout<<data;

        //std::string id(zmq_msg_gets(&receive1, "ID"));
        //const char* id = zmq_msg_gets(&receive1, ZMQ_MSG_PROPERTY_USER_ID);
        //const char* id = zmq_msg_gets(&receive1, "ID");
        //std::cout<<id<<std::endl;
//        sleep (1);          //  Do some 'work'
//        //zmq_send (responder, "World", 5, 0);
//        zmq_msg_t msg;
//        //zmq_msg_set_routing_id(&msg, 1111);
//        zmq_msg_init_size(&msg, 5);
//        memcpy(zmq_msg_data(&msg),"HELLO",5);
//        rc = zmq_msg_send (&msg, responder, 0);
    }
    return 0;
}
