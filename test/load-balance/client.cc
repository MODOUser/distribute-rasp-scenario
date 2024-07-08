#include"./../../src/load-balance/lbclient.h"
int main(){
    LBClient client("tcp://localhost");
    client.ToBroker();
    return 0;
}
