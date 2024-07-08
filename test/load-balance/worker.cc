#include"./../../src/load-balance/lbworker.h"
int main(){
    LBWorker worker("tcp://localhost");
    worker.EventLoop();
    return 0;
}
