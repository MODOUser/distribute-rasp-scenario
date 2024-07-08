#include"lbhelper.h"
static void worker_thread() {
    void *context = zmq_ctx_new();
    void *worker = zmq_socket(context, ZMQ_REQ);

    s_set_id(worker);
    zmq_connect(worker, "tcp://localhost:5673"); // backend

    //  Tell backend we're ready for work
    zmq_send(worker, "READY", 5, 0);
    std::cout<<s_recv(worker);

//    while (1) {
//        //  Read and save all frames until we get an empty frame
//        //  In this example there is only 1 but it could be more
//        std::string address = s_recv(worker);
//        {
//            std::string empty = s_recv(worker);
//            assert(empty.size() == 0);
//        }
//
//        //  Get request, send reply
//        //std::string request = s_recv(worker);
//        zmq_msg_t request;
//        zmq_msg_init(&request);
//        zmq_msg_recv(&request, worker, 0);
//        std::cout << "Worker: " << (const char *)zmq_msg_data(&request) << std::endl;
//
//        zmq_send (worker, address.c_str(), strlen (address.c_str()), ZMQ_SNDMORE);
//        zmq_send (worker, "", 0, ZMQ_SNDMORE);
//        zmq_send (worker, "OK", 2, ZMQ_SNDMORE);
//    }
}
int main(){
    worker_thread();
}
