#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string>
#include <list>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fstream>
#include <curl/curl.h>
#include <array>
#include <regex>
#include <cmath>

std::shared_ptr<spdlog::logger> g_logger;

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Minutes = std::chrono::minutes;
using Seconds = std::chrono::seconds;
using Milliseconds = std::chrono::milliseconds;

struct PacketInfo
{
    std::string packet_id;
    std::string client_id;
    TimePoint send_time;
    TimePoint receive_time;
    std::vector<char> data;
    bool retransmitted;
};

std::string serialize(const PacketInfo &packet)
{

    auto ms = std::chrono::duration_cast<Milliseconds>(packet.send_time.time_since_epoch()).count();
    std::ostringstream oss;
    oss << packet.packet_id << '|' << packet.client_id << '|' << ms << '|'
        << std::string(packet.data.begin(), packet.data.end()) << '|' << packet.retransmitted;
    return oss.str();
}

PacketInfo deserialize(const std::string &serialized)
{
    std::istringstream iss(serialized);
    std::string part;
    std::getline(iss, part, '|');
    std::string packet_id = part;

    std::getline(iss, part, '|');
    std::string client_id = part;

    std::getline(iss, part, '|');
    long long ms = std::stoll(part);
    TimePoint send_time = Clock::time_point(Milliseconds(ms));

    std::getline(iss, part, '|');
    std::vector<char> data(part.begin(), part.end());

    std::getline(iss, part, '|');
    bool retransmitted = std::stoi(part);

    return {packet_id, client_id, send_time, {}, data, retransmitted};
}

void fill_random_data(std::vector<char> &data)
{
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::generate(data.begin(), data.end(), [&]()
                  {
                      return charset[rand() % (sizeof(charset) - 1)]; // 减1以避免包括终止字符
                  });
}

std::string get_identity(void *socket)
{
    char identity[256];
    size_t identity_size = 256;
    zmq_getsockopt(socket, ZMQ_IDENTITY, identity, &identity_size);
    identity[identity_size] = '\0'; // 确保字符串正确终止
    return std::string(identity);
}

PacketInfo prepare_packet(void *socket, int batch_id, TimePoint send_time = Clock::now(), int data_size = 1024 * (rand() % 20 + 1), bool retransmitted = false)
{
    PacketInfo packet;
    packet.client_id = get_identity(socket);
    packet.packet_id = std::to_string(batch_id);
    packet.send_time = send_time;
    packet.retransmitted = retransmitted;

    packet.data.resize(data_size);
    fill_random_data(packet.data);

    return packet;
}

void send_packet(void *socket, const PacketInfo &packet)
{
    auto packet_str = serialize(packet);
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, packet_str.size());
    memcpy(zmq_msg_data(&msg), packet_str.data(), packet_str.size());
    zmq_msg_send(&msg, socket, 0);
    zmq_msg_close(&msg);
}

void zmq_send_ack(void *socket, const std::string &client_addr)
{
    zmq_send(socket, client_addr.c_str(), client_addr.size(), ZMQ_SNDMORE);
    zmq_send(socket, "", 0, ZMQ_SNDMORE);
    zmq_send(socket, "OK", 2, 0);
}

void zmq_request_retransmit(void *socket, const std::string &client_addr, const std::string &packet_id)
{
    zmq_send(socket, client_addr.c_str(), client_addr.size(), ZMQ_SNDMORE);
    zmq_send(socket, "", 0, ZMQ_SNDMORE);
    zmq_send(socket, ("RETRANSMIT|" + packet_id).c_str(), ("RETRANSMIT|" + packet_id).size(), 0);
}

int calculate_time(const TimePoint &start_time, const TimePoint &end_time)
{
    return std::chrono::duration_cast<Seconds>(end_time - start_time).count();
}
int calculate_time_ms(const TimePoint &start_time, const TimePoint &end_time)
{
    return std::chrono::duration_cast<Milliseconds>(end_time - start_time).count();
}

std::string calculate_sha256(const std::string &data)
{
    EVP_MD_CTX *mdctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int lengthOfHash = 0;

    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(mdctx, data.c_str(), data.size());
    EVP_DigestFinal_ex(mdctx, hash, &lengthOfHash);
    EVP_MD_CTX_free(mdctx);

    std::string result;
    for (unsigned int i = 0; i < lengthOfHash; ++i)
    {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        result += buf;
    }
    return result;
}

void setup_logging()
{
    // 创建并配置控制台 sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info); // 只在控制台输出 info 级别以上的日志
    console_sink->set_pattern("[%^%L%$] - %v");

    // 创建并配置文件 sink
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("test.log", true);
    file_sink->set_level(spdlog::level::trace); // 文件记录所有级别的日志
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%L%$] - %v - %t - %P");

    // 创建 logger 并添加两个 sink
    g_logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, file_sink});
    g_logger->set_level(spdlog::level::trace);

    // 设置自动刷新
    spdlog::flush_every(Seconds(3));
}

void export_data_to_csv(const std::string &filename, std::unordered_map<std::string, std::unordered_map<std::string, PacketInfo>> data_storage)
{
    std::ofstream csv_file(filename);

    if (!csv_file.is_open())
    {
        g_logger->error("Failed to open file: {}, timestamp: {}", filename, TimePoint_to_timestamp(Clock::now()));
        return;
    }

    csv_file << "Packet ID,Client ID,Send Time,Receive Time,Data Size,Retransmitted\n";

    // 遍历数据存储
    for (const auto &client_entry : data_storage)
    {
        const auto &client_id = client_entry.first;
        for (const auto &packet_entry : client_entry.second)
        {
            const PacketInfo &packet_info = packet_entry.second;
            auto send_ms = std::chrono::duration_cast<Milliseconds>(packet_info.send_time.time_since_epoch()).count();
            auto receive_ms = std::chrono::duration_cast<Milliseconds>(packet_info.receive_time.time_since_epoch()).count();
            int data_size = packet_info.data.size();
            bool retransmitted = packet_info.retransmitted;

            // 写入每个数据包的信息
            csv_file << packet_entry.first << ',' << client_id << ','
                     << send_ms << ',' << receive_ms << ',' << data_size << ','
                     << retransmitted << '\n';
        }
    }

    csv_file.close();
    g_logger->info("Data exported successfully to {}, timestamp: {}", filename, TimePoint_to_timestamp(Clock::now()));
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string get_public_ip()
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "";
        }
    }
    return readBuffer;
}

std::string exec_command(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

std::string get_public_ipv6_ip()
{
    std::string ipCommandOutput = exec_command("ip -6 addr show scope global");
    std::regex ipv6Regex("inet6 ([0-9a-fA-F:]+)/\\d{1,3} scope global");
    std::smatch match;
    if (std::regex_search(ipCommandOutput, match, ipv6Regex) && match.size() > 1)
    {
        return match[1].str(); // 返回第一个匹配的IPv6地址
    }
    return "No public IPv6 address found";
}

double get_data_size(const PacketInfo &packet)
{
    // 获取数据向量的大小（字节）
    size_t size_in_bytes = packet.data.size();

    // 转换为千字节 (KB) 并且直接保留两位小数
    double size_in_KB = size_in_bytes / 1024.0;
    return std::round(size_in_KB * 100.0) / 100.0;
}

std::string TimePoint_to_timestamp(const TimePoint &tp)
{
    // 将时间点转换为毫秒级的时间戳
    auto milliseconds = std::chrono::duration_cast<Milliseconds>(
                            tp.time_since_epoch())
                            .count();

    // 将毫秒时间戳转换为字符串
    return std::to_string(milliseconds);
}