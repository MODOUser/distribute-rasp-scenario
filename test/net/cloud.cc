/*
 * @Author: modouer
 * @Date: 2024-06-11 17:24:22
 * @LastEditors: modouer
 * @LastEditTime: 2024-07-12 10:23:03
 * @FilePath: /distribute-rasp-scenario/test/net/cloud.cc
 * @Description:
 */
#include "lbhelper.h"
#include "utils.h"

const int NORMAL_TRANSMIT_TIME = 1000;
const int RETRANSMIT_THRESHOLD = NORMAL_TRANSMIT_TIME;
const int LOSS_THRESHOLD = NORMAL_TRANSMIT_TIME;

std::atomic<bool> keep_running(true);

static void cloud()
{
    void *context = zmq_ctx_new();
    void *cloud = zmq_socket(context, ZMQ_ROUTER);
    void *cloud_send = zmq_socket(context, ZMQ_REQ);
    zmq_bind(cloud, "tcp://*:6000"); // 绑定到边端连接
    s_set_id(cloud_send);            // 设置可打印的身份
    zmq_connect(cloud_send, "tcp://localhost:5673");
    std::string cloud_addr = get_identity(cloud_send);
    zmq_pollitem_t items[] = {{cloud, 0, ZMQ_POLLIN, 0}};

    while (keep_running)
    {

        zmq_poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN)
        {
            // 接收边端请求
            std::string edge_addr = s_recv(cloud);
            std::string empty = s_recv(cloud);

            zmq_msg_t data;
            zmq_msg_init(&data);
            zmq_msg_recv(&data, cloud, 0);

            std::string packet((char *)zmq_msg_data(&data), zmq_msg_size(&data));
            zmq_msg_close(&data);

            PacketInfo received_packet = deserialize(packet);

            zmq_send_ack(cloud, edge_addr);

            std::srand(std::time(nullptr));
            // 生成 0.1 到 1 之间的随机毫秒数
            int randomMilliseconds = 100 + std::rand() % 901;
            std::this_thread::sleep_for(Milliseconds(randomMilliseconds));

            send_packet(cloud_send, received_packet);
            g_logger->info("Cloud {} sent: sample packet {}", cloud_addr, received_packet.packet_id);

            //
            // if (received_hash != calculated_hash)
            // {
            //     packet_info.retransmitted = true;
            //     zmq_request_retransmit(cloud, edge_addr, packet_id);
            //     continue;
            // }
            char *reply = s_recv(cloud_send);
            if (reply)
            {
                std::string reply_str(reply);
                free(reply);
                if (reply_str == "OK")
                {
                    g_logger->info("Cloud {} received: OK", cloud_addr);
                }
            }
            else
            {
                g_logger->error("Cloud {} failed to receive reply", cloud_addr);
            }
        }
    }
    zmq_close(cloud);
    zmq_close(cloud_send);
    zmq_ctx_destroy(context);
}

int main()
{
    setup_logging();

    std::thread cloud_thread(cloud);
    std::this_thread::sleep_for(Minutes(12));

    // 通知云端线程停止
    keep_running = false;

    // 等待云端线程结束
    if (cloud_thread.joinable())
    {
        cloud_thread.join();
    }

    return 0;
}
