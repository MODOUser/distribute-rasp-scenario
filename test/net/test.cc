#include "lbhelper.h"
#include "utils.h"
#include <iostream>
#include <unistd.h>
#include <chrono>

using Clock = std::chrono::system_clock;
using Seconds = std::chrono::seconds;
using TimePoint = std::chrono::time_point<Clock>;

int calculate_time(const TimePoint &start_time, const TimePoint &end_time)
{
    return std::chrono::duration_cast<Seconds>(end_time - start_time).count();
}

int main()
{
    auto now = Clock::now();
    std::cout << "Current timestamp: " << std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() << " seconds since epoch" << std::endl;
    sleep(5); // 等待5秒
    auto now1 = Clock::now();
    std::cout << "Elapsed time: " << calculate_time(now, now1) << " seconds" << std::endl;
    return 0;
}
