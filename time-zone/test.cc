#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include<iostream>
using namespace std;
int main()
{
        FILE* fp = NULL;
        char cmd[512];
        sprintf(cmd, "cat /proc/cpuinfo | grep -c \"processor\"");
        if ((fp = popen(cmd, "r")) != NULL)
        {
                fgets(cmd, sizeof(cmd), fp);
                                pclose(fp);
        }

                  //0 成功， 1 失败
        cout<<stoi(cmd);
        return 0;
}
