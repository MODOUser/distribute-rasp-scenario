#include"./../../../libzmq/src/clock.hpp"
#include <unistd.h>
#include<iostream>
int main(){
    auto now = zmq::clock_t::now_us ();
    usleep(100);
    auto next = zmq::clock_t::now_us();
    std::cout<<now<<'\t'<<next;
    return 0;
}
