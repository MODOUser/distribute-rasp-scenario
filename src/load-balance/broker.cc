#include"lbhelper.h"
#include<queue>
static void broker_thread(){
    //  Prepare our context and sockets
    void *context_t = zmq_ctx_new();
    void *frontend = zmq_socket(context_t, ZMQ_ROUTER);
    void *backend = zmq_socket(context_t, ZMQ_ROUTER);
    std::queue<std::string> worker_queue;

    zmq_bind(frontend, "tcp://*:5672"); // frontend
    zmq_bind(backend, "tcp://*:5673"); // backend

    //  Logic of LRU loop
    //  - Poll backend always, frontend only if 1+ worker ready
    //  - If worker replies, queue worker as ready and forward reply
    //    to client if necessary
    //  - If client requests, pop next worker and send request to it
    //
    //  A very simple queue structure with known max size
    //  Initialize poll set
    zmq_pollitem_t items [2];
    /* First item refers to 0MQ socket 'socket' */
    items[0].socket = backend;
    items[0].events = ZMQ_POLLIN;
    /* Second item refers to standard socket 'fd' */
    items[1].socket = frontend;
    items[1].events = ZMQ_POLLIN;

    while (1) {

        zmq_poll (items, 2, -1);
//        if (worker_queue.size())
//            zmq_poll(items, 2, -1);
//        else
//            zmq_poll(&items[1], 1, -1);

        //  Handle worker activity on backend
        if (items[0].revents & ZMQ_POLLIN) {
            std::cout<<"worker 发消息了！"<<std::endl;

            //  Queue worker address for LRU routing
            worker_queue.push(s_recv(backend));


            {
                //  Second frame is empty
                std::string empty = s_recv(backend);
                assert(empty.size() == 0);
            }

            //  Third frame is READY or else a client reply address
            std::string client_addr = s_recv(backend);

            std::cout<<"worker ready "<<std::endl;

            //  If client reply, send rest back to frontend
//            if (client_addr.compare("READY") != 0) {
//
//                {
//                    std::string empty = s_recv(backend);
//                    assert(empty.size() == 0);
//                }
//
//                std::string reply = s_recv(backend);
//                zmq_send (frontend, client_addr.c_str(), strlen (client_addr.c_str()), ZMQ_SNDMORE);
//                zmq_send (frontend,  "", 0, ZMQ_SNDMORE);
//                zmq_send (frontend, reply.c_str(), strlen(reply.c_str()), 0);
//
////                if (--client_nbr == 0)
////                    break;
//            }
        }
        if (items[1].revents & ZMQ_POLLIN) {

            //  Now get next client request, route to LRU worker
            //  Client request is [address][empty][request]
            std::string client_addr = s_recv(frontend);
            std::cout<<client_addr<<std::endl;

            {
                std::string empty = s_recv(frontend);
                assert(empty.size() == 0);
                std::cout<<"空"<<std::endl;

            }

            //std::string request = s_recv(frontend);
            zmq_msg_t request;
            zmq_msg_init(&request);
            zmq_msg_recv(&request, frontend, 0);
            //std::string test(s_recv(frontend));
            

            std::cout<<" "<<client_addr<<'\t'<<(const char *)zmq_msg_data(&request)<<std::endl;
            //zmq_send (frontend, client_addr.c_str(), strlen (client_addr.c_str()), ZMQ_SNDMORE);
            zmq_send (frontend, "CC66-C879", 9, ZMQ_SNDMORE);
            zmq_send (frontend, "", 0, ZMQ_SNDMORE);
            zmq_send (frontend, "OK", 2, 0);


//            std::string worker_addr = worker_queue.front();//worker_queue [0];
//            worker_queue.pop();
//
//            s_sendmore(backend, worker_addr);
//            s_sendmore(backend, "");
//            s_sendmore(backend, client_addr);
//            s_sendmore(backend, "");
//            s_send(backend, request);
        }
    }
}
int main(){
    broker_thread();
    return 0;
}
