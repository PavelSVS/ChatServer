/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BoostTCPServer.h
 * Author: dmitry
 *
 * Created on 26 мая 2021 г., 16:07
 */

#ifndef BOOSTTCPSERVER_H
#define BOOSTTCPSERVER_H
#include <boost/asio.hpp>
#include <optional>
#include <queue>
#include <functional>
#include <unordered_set>
#include <pthread.h>
#include <mutex>
#include <iostream>
#include "BoostTCPSession.h"
#include "BoostTCPServer.h"
#include "Logger.h"

namespace io = boost::asio;
using tcp = io::ip::tcp;
using error_code = boost::system::error_code;
using namespace std::placeholders;

class BoostTCPServer{
    public:
        BoostTCPServer(io::io_service& io_context, std::uint16_t port);
        void async_accept();
        int init(void* arg);
        void setDataHandlerCallback(std::function<void*(int, void*, int, char[], void*)>);
        void setOnCloseCallback(std::function<void(int, void*)>);
        int send(char* data, int len, int socket);
        int send(std::string s, int socket);
        int broadcast(std::string s);
        void forceCloseSocket(int socket);
        
    private:
        void* aux;
        io::io_service& io_context;
        tcp::acceptor acceptor;
        pthread_t Listener; // Main Server Listener

        void onMessage(std::string const& message, std::string const& ip, int const& socket);
        static void* Listener_process(void *arg); 
        void onClose(int socket);
        std::optional<tcp::socket> socket;
        std::unordered_set<std::shared_ptr<TCPSession>> clients;
        std::mutex clientsMutex;
        
        std::function<void*(int, void*, int, char[], void*)> data_handler_callback;
        std::function<void(int, void*)> on_close_callback;
        
};



#endif /* BOOSTTCPSERVER_H */

