#include "lbhelper.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

int
main(int argc, char *argv[])
{
    std::cout<<get_ip()<<std::endl;
    return 0;
}
