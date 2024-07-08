#include<zmq.hpp>
#include<iostream>
using namespace zmq;
int main(){
    zmq::context_t context (1);
    zmq::socket_t socket (context, zmq::socket_type::req);

    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect ("tcp://localhost:5555");
//    context_t context(1);
//    socket_t socket(context,zmq::socket_type::req);
//
//    socket.connect("tcp://localhost:5555");
    
    for(int i = 0; i != 50; ++i){
//        zmq::message_t request (5);
//        memcpy (request.data (), "Hello", 5);
//        std::cout << "Sending Hello " << i << "..." << std::endl;
//        socket.send (request, zmq::send_flags::none);
        message_t message(1024);
        std::string s(1024,'*');
        memcpy(message.data(), &s, 1024);
        socket.send(message,zmq::send_flags::none);

//        zmq::message_t reply;
//        socket.recv (reply, zmq::recv_flags::none);
//        std::cout << reply.to_string() << std::endl;
    }
    return 0;
}
