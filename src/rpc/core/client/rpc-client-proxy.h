/*
 * 发送时需要创建代理。进行Request赋值，调用invoke。
 *
 * */
#ifndef RPC_CLIENT_PROXY_H
#define RPC_CLIENT_PROXY_H
#include<iostream>
#include"./../../idl/task/task-request.h"
#include"./../../idl/task/task-response.h"
#include"./../../idl/task/task-service.h"
using namespace std;

class RpcClientProxy{
    public:
        RpcClientProxy(TaskRequest req,TaskService ser):m_requeset(req),m_service(ser){}
        void Invoke();

    private:
        
        TaskRequest m_requeset;
        TaskResponse m_response;
        TaskService m_service;

};
#endif
