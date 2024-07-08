#include"lbhelper.h"
#include"./../thread-pool/thread-pool.h"
#include<unistd.h>
static void client_thread() {

    void *context_t = zmq_ctx_new();
    void *client = zmq_socket(context_t, ZMQ_REQ);
    s_set_id(client); // Set a printable identity
    zmq_connect(client, "tcp://localhost:5672"); // frontend
    while(1){
        //  Send request, get reply
//        zmq_send (client, "HELLO", 5, 0);
        zmq_msg_t msg;
        zmq_msg_init_size(&msg, 5);
        memcpy(zmq_msg_data(&msg), "HELLO", 5);
        zmq_msg_send(&msg, client, 0);
        std::cout<<"Client send hello "<<std::endl;
        //zmq_send(client, "HHH", 3, 0);
        sleep(1);
        std::string reply = s_recv(client);
        std::cout << "Client: " << reply << std::endl;
    }
    
}
int main(){
//    ThreadPool pool(100);
//    pool.init();
//    for(int i = 0; i < 99; ++i){
//        pool.submit(client_thread);
//    }
    client_thread();
    return 0;
}
