#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
int main (int argc, char **argv)
{
    //printf ("Connecting to hello world server…\n");
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://localhost:5555");
    //zmq_setsockopt(requester,ZMQ_IDENTITY, "ssf666",6);
    
    //std::cout<<zmq_getsockopt(requester,ZMQ_IDENTITY,)

    int request_nbr;
    for (request_nbr = 0; request_nbr != 10; request_nbr++) {
        //char buffer [10];
        //printf ("Sending Hello %d…\n", request_nbr);
        zmq_msg_t msg;
        //zmq_msg_set_routing_id(&msg, 1111);
        zmq_msg_init_size(&msg, 5);
        memcpy(zmq_msg_data(&msg),argv[1],5);
        int rc = zmq_msg_send (&msg, requester, 0);
        //std::cout<<rc<<std::endl;
        zmq_msg_t recv;
        zmq_msg_init(&recv);
        zmq_msg_recv(&recv,requester,0);
        printf ("Received World %d\n", request_nbr);
    }
    zmq_close (requester);
    zmq_ctx_destroy (context);
    return 0;
}
