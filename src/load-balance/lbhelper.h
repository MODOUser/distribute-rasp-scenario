#ifndef LBHELPER_H
#define LBHELPER_H
#include <zmq.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <cassert>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <hiredis/hiredis.h>
#include <functional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <random>
#include <sstream>
#include "./../thread-pool/thread-pool.h"
#include "./../ftp/file-transfer.h"
#define randof(num) (int)((float)(num) * random() / (RAND_MAX + 1.0))
static void s_set_id(void *socket)
{
    static std::random_device rd;                          // 非确定性随机数生成器
    static std::mt19937 gen(rd());                         // 基于 Mersenne Twister 的随机数生成器
    static std::uniform_int_distribution<> dis(0, 0xFFFF); // 均匀分布

    // 生成随机ID
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << dis(gen)
       << "-" << std::setw(4) << std::setfill('0') << dis(gen);

    std::string identity = ss.str();
    zmq_setsockopt(socket, ZMQ_IDENTITY, identity.c_str(), identity.size());
}

// static std::string s_set_id(void *socket)
// {
//     char identity[9];
//     sprintf(identity, "%04X-%04X", randof(0x10000), randof(0x10000));
//     zmq_setsockopt(socket, ZMQ_IDENTITY, identity, strlen(identity));
//     return std::string(identity);
// }
static void
s_set_id(void *socket, char identity[10])
{
    srand(time(NULL));
    sprintf(identity, "%04X-%04X", rand() % 0x10000, rand() % 0x10000);
    zmq_setsockopt(socket, ZMQ_IDENTITY, identity, strlen(identity));
}

static char *s_recv(void *socket)
{
    enum
    {
        cap = 256
    };
    char buffer[cap];
    int size = zmq_recv(socket, buffer, cap - 1, 0);
    if (size == -1)
        return NULL;
    buffer[size < cap ? size : cap - 1] = '\0';

    return strndup(buffer, sizeof(buffer) - 1);
    // remember that the strdup family of functions use malloc/alloc for space for the new string.  It must be manually
    // freed when you are done with it.  Failure to do so will allow a heap attack.
}

// 精确到ms级别
uint64_t
clock_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (uint64_t)((int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000);
}

std::string get_ip()
{ // 按照文本格式输出的，raspOS可能还需要修改
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET)
        {
            std::string ip(ifa->ifa_name);
            if (ip.compare(0, 3, "enp") == 0)
            {
                return inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
            }
            // printf("interfac: %s, ip: %s\n", ifa->ifa_name, inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr));
        }
    }
    freeifaddrs(ifaddr);
    return "";
}

#endif
