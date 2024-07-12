/*
 * @Author: modouer
 * @Date: 2024-06-11 17:24:22
 * @LastEditors: modouer
 * @LastEditTime: 2024-07-10 22:26:20
 * @FilePath: /distribute-rasp-scenario/test/net/server.cc
 * @Description:
 */
#include "lbhelper.h"
#include "utils.h"

const int NORMAL_TRANSMIT_TIME = 1000;
const int RETRANSMIT_THRESHOLD = NORMAL_TRANSMIT_TIME;
const int LOSS_THRESHOLD = NORMAL_TRANSMIT_TIME;
std::unordered_map<std::string, std::unordered_map<std::string, PacketInfo>> data_storage;

std::atomic<bool> keep_running(true);

static void edge()
{
    void *context = zmq_ctx_new();
    void *frontend = zmq_socket(context, ZMQ_ROUTER);
    // s_set_ipv6(frontend);
    zmq_bind(frontend, "tcp://*:5672"); // 绑定到客户端连接
    zmq_pollitem_t items[] = {{frontend, 0, ZMQ_POLLIN, 0}};

    while (keep_running)
    {

        zmq_poll(items, 1, 100);

        if (items[0].revents & ZMQ_POLLIN)
        {
            // 接收客户端请求
            std::string client_addr = s_recv(frontend);
            std::string empty = s_recv(frontend);

            zmq_msg_t data;
            zmq_msg_init(&data);
            zmq_msg_recv(&data, frontend, 0);

            std::string packet((char *)zmq_msg_data(&data), zmq_msg_size(&data));
            zmq_msg_close(&data);

            PacketInfo received_packet = deserialize(packet);
            auto now = Clock::now();

            if (data_storage[received_packet.client_id].find(received_packet.packet_id) == data_storage[received_packet.client_id].end())
            {
                received_packet.receive_time = now;
                data_storage[received_packet.client_id][received_packet.packet_id] = received_packet;
            }
            //
            // if (received_hash != calculated_hash)
            // {
            //     packet_info.retransmitted = true;
            //     zmq_request_retransmit(frontend, client_addr, packet_id);
            //     continue;
            // }

            if (received_packet.retransmitted)
            {
                PacketInfo &packet_info = data_storage[received_packet.client_id][received_packet.packet_id];
                auto transmission_time = calculate_time_ms(packet_info.send_time, now);
                if (transmission_time < LOSS_THRESHOLD + RETRANSMIT_THRESHOLD)
                {
                    packet_info.receive_time = now;
                    g_logger->info("Edge received packet on time from {}: {}, packet size: {}KB, transmission time: {}ms (retransmit)", client_addr, packet_info.packet_id, get_data_size(packet_info), calculate_time_ms(packet_info.send_time, packet_info.receive_time));
                    zmq_send_ack(frontend, client_addr);
                }
                else
                {
                    g_logger->warn("Retransmit packet lost for {}: {}", client_addr, packet_info.packet_id);
                    data_storage[packet_info.client_id].erase(packet_info.packet_id);
                    zmq_send_ack(frontend, client_addr);
                }
            }
            else
            {
                auto transmission_time = calculate_time_ms(received_packet.send_time, now);
                if (transmission_time < RETRANSMIT_THRESHOLD)
                {
                    g_logger->info("Edge received packet on time from {}: {}, packet size: {}KB, transmission time: {}ms (regular)", client_addr, received_packet.packet_id, get_data_size(received_packet), calculate_time_ms(received_packet.send_time, received_packet.receive_time));
                    zmq_send_ack(frontend, client_addr);
                }
                else
                {
                    g_logger->warn("Edge received timeout, requesting retransmit from {}: {}, transmission time: {}ms", client_addr, received_packet.packet_id, calculate_time_ms(received_packet.send_time, received_packet.receive_time));
                    data_storage[received_packet.client_id][received_packet.packet_id].retransmitted = true;
                    zmq_request_retransmit(frontend, client_addr, received_packet.packet_id);
                }
            }
        }
    }
    zmq_close(frontend);
    zmq_ctx_destroy(context);
}

int main()
{

    setup_logging();

    std::thread server_thread(edge);
    std::this_thread::sleep_for(Minutes(12));

    // 通知服务器线程停止
    keep_running = false;

    // 等待服务器线程结束
    if (server_thread.joinable())
    {
        server_thread.join();
    }

    // 处理data_storage中的数据
    export_data_to_csv("received_packets_data.csv", data_storage);
    return 0;
}
