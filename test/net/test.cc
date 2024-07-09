/*
 * @Author: modouer
 * @Date: 2024-07-01 07:06:12
 * @LastEditors: modouer
 * @LastEditTime: 2024-07-09 14:12:07
 * @FilePath: /distribute-rasp-scenario/test/net/test.cc
 * @Description:
 */
#include "lbhelper.h"
#include "utils.h"
#include <iostream>
#include <unistd.h>
#include <chrono>

int main()
{
    // auto now = Clock::now();
    // std::cout << "Current timestamp: " << std::chrono::duration_cast<Seconds>(now.time_since_epoch()).count() << " seconds since epoch" << std::endl;
    // sleep(5); // 等待5秒
    // auto now1 = Clock::now();
    // std::cout << "Elapsed time: " << calculate_time_ms(now, now1) << " seconds" << std::endl;
    // return 0;
    // std::string ip = get_public_ip();
    // std::cout << "Public IP Address: " << ip << std::endl;
    // return 0;
    try
    {
        std::string ipv6Address = get_public_ipv6_ip();
        std::cout << "Public IPv6 Address: " << ipv6Address << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
