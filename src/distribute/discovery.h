#ifndef DISCOVERY_H
#define DISCOVERY_H
#include<zmq.h>
#include<iostream>
#include"../EasiEI.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <hiredis/hiredis.h>
using namespace std;
#define HEARTBEAT_LIVENESS 3 // 3-5 is reasonable 
#define HEARTBEAT_INTERVAL 1000 // msec // Paranoid Pirate Protocol constants 
#define PPP_READY "\001" // Signals worker is ready 
#define PPP_HEARTBEAT "\002" // Signals worker heartbeat
int Broker(){
    void *ctx = zmq_ctx_new();
    void *frontend = zmq_socket (ctx, ZMQ_ROUTER);
    void *backend = zmq_socket (ctx, ZMQ_ROUTER);
    zmq_bind (frontend, "tcp://*:5555");
    // For clients 
    zmq_bind (backend, "tcp://*:5556");
    // For workers 
    // List of available workers 这里修改用Metric table进行保存
    zlist_t *workers = zlist_new ();
    // Send out heartbeats at regular intervals 
    // 时间获取要改一下
    uint64_t heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;

    while (true) { 
        zmq_pollitem_t items [] = { { backend, 0, ZMQ_POLLIN, 0 }, { frontend, 0, ZMQ_POLLIN, 0 } };
        // Poll frontend only if we have available workers 
        int rc = zmq_poll (items, zlist_size (workers)? 2: 1,HEARTBEAT_INTERVAL * ZMQ_POLL_MSEC);
        if (rc == -1) break;
        // Interrupted 
        // Handle worker activity on backend 
        if (items [0].revents & ZMQ_POLLIN) { // Use worker identity for load balancing 
            zmq_msg_t *msg = zmq_msg_recv (backend);
            if (!msg) break;
            // Interrupted // Any sign of life from worker means it's ready 
            // 更新或添加worker
            zframe_t *identity = zmq_msg_unwrap (msg);
            worker_t *worker = s_worker_new (identity);
            s_worker_ready (worker, workers);
            // Validate control message, or return reply to client 
            // 若worker发送的是感知信息则不需要传递给client，若worker发送的是执行结果则要转发给client
            if (zmq_msg_size (msg) == 1) { zframe_t *frame = zmq_msg_first (msg);
                if (memcmp (zframe_data (frame), PPP_READY, 1) && memcmp (zframe_data (frame), PPP_HEARTBEAT, 1)) { printf ("E: invalid message from worker");
                    zmq_msg_dump (msg);
                } zmq_msg_destroy (&msg);
            } else zmq_msg_send (&msg, frontend);
        } if (items [1].revents & ZMQ_POLLIN) { // Now get next client request, route to next worker 
            zmq_msg_t *msg = zmq_msg_recv (frontend);
            if (!msg) break;
            // Interrupted 
            zmq_msg_push (msg, s_workers_next (workers));
            zmq_msg_send (&msg, backend);
        }if (zclock_time () >= heartbeat_at) { worker_t *worker = (worker_t *) zlist_first (workers);
            while (worker) { zframe_send (&worker->identity, backend, ZFRAME_REUSE + ZFRAME_MORE);
                zframe_t *frame = zframe_new (PPP_HEARTBEAT, 1);
                zframe_send (&frame, backend, 0);
                worker = (worker_t *) zlist_next (workers);
            }heartbeat_at = zclock_time () + HEARTBEAT_INTERVAL;
        } s_workers_purge (workers);
    } // When we're done, clean up properly 
    while (zlist_size (workers)) { worker_t *worker = (worker_t *) zlist_pop (workers);
        s_worker_destroy (&worker);
    } 
    zlist_destroy (&workers);
    zctx_destroy (&ctx);
return 0;}

#endif
