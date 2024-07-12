/*
 * @Author: modouer
 * @Date: 2024-06-11 17:24:12
 * @LastEditors: modouer
 * @LastEditTime: 2024-07-12 20:02:27
 * @FilePath: /distribute-rasp-scenario/test/net/client-cloud.cc
 * @Description:
 */
#include "lbhelper.h"
#include "utils.h"
#include <thread>

const int MAX_STORAGE_SIZE = 5;
const int NORMAL_TRANSMIT_TIME = 2000;
const int RETRANSMIT_THRESHOLD = NORMAL_TRANSMIT_TIME;
const int LOSS_THRESHOLD = NORMAL_TRANSMIT_TIME;

std::unordered_map<std::string, std::list<PacketInfo>> packet_storage;
std::atomic<int> packet_counter(0);

std::atomic<bool> keep_running(true);

static void client(int id)
{
    void *context = zmq_ctx_new();
    void *client = zmq_socket(context, ZMQ_REQ);
    s_set_id(client); // 设置可打印的身份
    zmq_connect(client, "tcp://192.168.149.183:5672");
    std::string client_addr = get_identity(client);
    while (keep_running)
    {
        PacketInfo packet = prepare_packet(client, ++packet_counter);
        // std::string hash = calculate_sha256(data);

        // 管理数据包缓存
        auto &queue = packet_storage[client_addr];
        if (queue.size() >= MAX_STORAGE_SIZE)
        {
            queue.pop_front(); // 如果缓存满了，移除最早的数据包
        }
        queue.push_back(packet);

        // sleep(rand() % 6 + 1);
        send_packet(client, packet);
        g_logger->info("Client {} sent: packet {}, timestamp: {}", client_addr, packet.packet_id, TimePoint_to_timestamp(packet.send_time));

        char *reply = s_recv(client);
        if (reply)
        {
            std::string reply_str(reply);
            free(reply);
            if (reply_str == "OK")
            {
                g_logger->info("Client {} received: OK, timestamp: {}", client_addr, TimePoint_to_timestamp(Clock::now()));
            }
            // else
            // {
            //     size_t delimiter_pos = reply_str.find('|');
            //     std::string packet_id = reply_str.substr(delimiter_pos + 1);
            //     auto it = std::find_if(packet_storage[client_addr].begin(), packet_storage[client_addr].end(),
            //                            [&](const PacketInfo &info)
            //                            { return info.packet_id == packet_id; });
            //     if (it != packet_storage[client_addr].end())
            //     {
            //         // 重传数据包
            //         it->retransmitted = true;
            //         send_packet(client, *it);
            //         g_logger->warn("Client {} retransmitted: packet {}", client_addr, it->packet_id);
            //         char *retransmitted_reply = s_recv(client);
            //         std::string retransmitted_reply_str(retransmitted_reply);
            //         free(retransmitted_reply);
            //         if (retransmitted_reply_str == "OK")
            //         {
            //             g_logger->info("Client {} retransmitted received: OK", client_addr);
            //         }
            //     }
            //     else
            //     {
            //         g_logger->warn("Client {} failed to retransmit: packet {} not found, maybe expired", client_addr, packet_id);
            //     }
            // }
        }
        else
        {
            g_logger->error("Client {} failed to receive reply, timestamp: {}", client_addr, TimePoint_to_timestamp(Clock::now()));
        }
        sleep(1);
    }
    g_logger->info("Client {} sent {} packets in total, timestamp: {}", client_addr, packet_counter, TimePoint_to_timestamp(Clock::now()));
    zmq_close(client);
    zmq_ctx_destroy(context);
}
int main()
{

    setup_logging();

    std::thread client_thread(client, 1);
    std::this_thread::sleep_for(Minutes(10));

    keep_running = false;

    if (client_thread.joinable())
    {
        client_thread.join();
    }
    return 0;
}

// int main()
// {
//     setup_logging();

//     int num_clients = 5;
//     std::vector<std::thread> threads;
//     // 启动多个客户端线程
//     for (int i = 0; i < num_clients; ++i)
//     {
//         threads.emplace_back(client, i + 1);
//     }

//     std::this_thread::sleep_for(Minutes(10));

//     keep_running = false;

//     for (auto &thread : threads)
//     {
//         if (thread.joinable())
//         {
//             thread.join();
//         }
//     }

//     return 0;
// }