#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>

int main (void)
{
    //  Socket to talk to clients
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, "tcp://*:5555");
    assert (rc == 0);

    while (1) {
        zmq_msg_t msg;
        zmq_msg_recv(&msg);
        uint8_t *data = static_cast<uint8_t *> (zmq_msg_data (&msg));
        uint16_t event = *reinterpret_cast<uint16_t *> (data);
        std::cout<<data<<'\t'<<event<<std::endl;
        printf ("Received Hello\n");
        sleep (1);          //  Do some 'work'
        zmq_send (responder, "World", 5, 0);
    }
    return 0;
}
