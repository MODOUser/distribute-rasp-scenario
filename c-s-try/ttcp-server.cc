#include<zmq.hpp>
#include<iostream>
#include <unistd.h>
using namespace zmq;
int main(){
    
    context_t context(2);
    socket_t socket(context,zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    while(1){
        message_t request;
        socket.recv(request,zmq::recv_flags::none);

        std::cout<<request.size()<<std::endl;

        //std::cout << "Received Hello" << std::endl;
        sleep(1);

        //  Send reply back to client
        zmq::message_t reply (5);
        memcpy (reply.data (), "World", 5);
        socket.send (reply, zmq::send_flags::none);
    }
    return 0;

}
