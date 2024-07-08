#include"./../../src/load-balance/lbbroker.h"
int main(){
    LBBroker broker("tcp://*");
    broker.EventLoop();
    return 0;
}
