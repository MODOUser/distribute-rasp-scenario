#include<zmq.h>
#include<iostream>
int main(){
    zmq_msg_t t;
    zmq_msg_init(&t,7);
    memcpy(zmq_msg_data(&t),"hhhhhhh",7);
    zmq_msg_set(&t,)

    return 0;
}
