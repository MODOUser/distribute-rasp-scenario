#include<zmq.h>
#include<stdio.h>
static void server_thread(void *args, void* context, void *pipe){
    File *file = fopen("client.cc","r");
    assert(file);

    void *router = zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(router, "tcp://*:6000");

    while(true){:q
        ：q:：
        
    }
}
