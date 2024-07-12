/*
 * @Author: modouer
 * @Date: 2024-06-11 17:24:22
 * @LastEditors: modouer
 * @LastEditTime: 2024-07-12 09:58:02
 * @FilePath: /distribute-rasp-scenario/test/net/server-cloud.cc
 * @Description:
 */
#include "lbhelper.h"
#include "utils.h"

const int NORMAL_TRANSMIT_TIME = 1000;
const int RETRANSMIT_THRESHOLD = 3 * NORMAL_TRANSMIT_TIME;
const int LOSS_THRESHOLD = 3 * NORMAL_TRANSMIT_TIME;
std::unordered_map<std::string, std::unordered_map<std::string, PacketInfo>> data_storage;
std::unordered_map<std::string, std::unordered_map<std::string, PacketInfo>> sample_storage;

std::atomic<bool> keep_running(true);
int latest_batch_number = 0;

static void edge()
{
    void *context = zmq_ctx_new();
    void *frontend = zmq_socket(context, ZMQ_ROUTER);
    void *backend = zmq_socket(context, ZMQ_ROUTER);
    void *edge_send = zmq_socket(context, ZMQ_REQ);

    zmq_bind(frontend, "tcp://*:5672"); // 绑定到客户端连接
    zmq_bind(backend, "tcp://*:5673");  // 绑定到云端连接
    s_set_id(edge_send);                // 设置可打印的身份
    zmq_connect(edge_send, "tcp://localhost:6000");
    std::string edge_addr = get_identity(edge_send);

    zmq_pollitem_t items[2];
    items[0].socket = frontend;
    items[0].events = ZMQ_POLLIN;

    items[1].socket = backend;
    items[1].events = ZMQ_POLLIN;

    while (keep_running)
    {

        zmq_poll(items, 2, 100);

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

            PacketInfo &packet_info = data_storage[received_packet.client_id][received_packet.packet_id];
            auto transmission_time = calculate_time_ms(packet_info.send_time, now);
            if (transmission_time < RETRANSMIT_THRESHOLD)
            {
                g_logger->info("Edge received packet on time from {}: {}, packet size: {}KB, transmission time: {}ms (regular)", client_addr, packet_info.packet_id, get_data_size(packet_info), calculate_time_ms(packet_info.send_time, packet_info.receive_time));
            }
            else
            {
                g_logger->warn("Packet lost for {}: {}", client_addr, packet_info.packet_id);
                data_storage[packet_info.client_id].erase(packet_info.packet_id);
            }
            zmq_send_ack(frontend, client_addr);

            if (std::stoi(received_packet.packet_id) > latest_batch_number)
            {
                latest_batch_number = std::stoi(received_packet.packet_id);
                PacketInfo sample_packet = prepare_packet(edge_send, latest_batch_number, received_packet.send_time, 1024 * 40);
                // std::string hash = calculate_sha256(data);

                send_packet(edge_send, sample_packet);
                // 处理新批次号的数据包
                g_logger->info("Edge {} sent: sample packet {} to cloud", edge_addr, latest_batch_number);
                char *reply = s_recv(edge_send);
                if (reply)
                {
                    std::string reply_str(reply);
                    free(reply);
                    if (reply_str == "OK")
                    {
                        g_logger->info("Edge {} received: OK", edge_addr);
                    }
                }
                else
                {
                    g_logger->error("Edge {} failed to receive reply", edge_addr);
                }
            }
        }
        if (items[1].revents & ZMQ_POLLIN)
        {

            // 接收云端请求
            std::string cloud_addr = s_recv(backend);
            std::string empty = s_recv(backend);

            zmq_msg_t data;
            zmq_msg_init(&data);
            zmq_msg_recv(&data, backend, 0);

            std::string cloud_packet((char *)zmq_msg_data(&data), zmq_msg_size(&data));
            zmq_msg_close(&data);

            PacketInfo received_sample_packet = deserialize(cloud_packet);
            auto now = Clock::now();

            if (sample_storage[received_sample_packet.client_id].find(received_sample_packet.packet_id) == sample_storage[received_sample_packet.client_id].end())
            {
                std::srand(std::time(nullptr));
                // 生成 0.1 到 0.3 之间的随机毫秒数
                int randomMilliseconds = 100 + std::rand() % 201;
                std::this_thread::sleep_for(Milliseconds(randomMilliseconds));
                received_sample_packet.receive_time = now;
                sample_storage[received_sample_packet.client_id][received_sample_packet.packet_id] = received_sample_packet;
            }
            g_logger->info("Edge {} received sample packet on time from {}: {}, packet size: {}KB, transmission time: {}ms (regular)", edge_addr, cloud_addr, received_sample_packet.packet_id, get_data_size(received_sample_packet), calculate_time_ms(received_sample_packet.send_time, received_sample_packet.receive_time));
            zmq_send_ack(backend, cloud_addr);
        }
    }
    zmq_close(frontend);
    zmq_close(backend);
    zmq_close(edge_send);
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
    export_data_to_csv("received_sample_packets_data.csv", sample_storage);
    return 0;
}
