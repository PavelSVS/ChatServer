/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebsocketServer.h
 * Author: dmitry
 *
 * Created on February 3, 2021, 6:09 PM
 */

#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H
#define BOOST_TCP_SERVER
#include <list>
#include <mutex>
#include <functional>
#include <optional>
#include <stdlib.h>
#include <iomanip>
#include <openssl/sha.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#ifdef BOOST_TCP_SERVER
    #include "BoostTCPServer.h"
#else
    #include "TCPServer.h"
#endif

#include "Logger.h"
#include "base64.h"
#include "WsClientData.h"

#define GUID                        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"      // GUID for websocket

using namespace std;

class WsClientData;

class WebsocketServer{

    struct packet_t{
        int len;
        uint8_t* data;
    };
    
    public:
#ifdef BOOST_TCP_SERVER
        BoostTCPServer* server;
#else
        TCPServer* server
#endif
        void* aux;
        
        WebsocketServer();
        ~WebsocketServer();
        int init(uint16_t port, string interface, void* arg);
        void setOnConnectedCallback(std::function<void(int, string, char[], void*)>);
        void setOnMessageCallback(std::function<void(int, void*, int, void*)>);
        void setOnCloseCallback(std::function<void(int, void*)>);
        void send(char* data, int len, int socket, bool binary);
        void broadcast(char* data, int len, bool binary);
        void broadcastInternal( char* data, int len, bool binary, bool pong);
        void close(int socket);

    private:
        uint32_t listenPort;
        string listenInterface;
        list<WsClientData> tempData;
        std::mutex tempDataMutex;
#ifdef BOOST_TCP_SERVER
        boost::asio::io_service io_context;
#endif
        void sendInternal(char* data, int len, int socket, bool binary, bool pong = false);
        std::optional<std::list<WsClientData>::iterator> getBufferIterator(int socket);
        WsClientData* newBuffer(int socket);
        WsClientData* getBuffer(int socket);
        void deleteBuffer(int socket);
        void cleanBuffer(int socket);
        int addToBuffer(int socket, char* data, int len);
        string getParameters(string req);
        string buildInitAnswer(string request);
        static void* socketData(int socket, void* buf, int nBytes, char ip[], void* arg);
        bool parseWebsocketBuffer(WsClientData* client, char ip[]);
        static void* socketClose(int socket, void* arg);
        void onConnected(int socket, string params, char ip[]);
        std::function<void(int, string, char[], void*)> on_connected_callback;
        std::function<void(int, void*, int, void*)> on_message_callback;
        std::function<void(int, void*)> on_close_callback;


};

#endif /* WEBSOCKETSERVER_H */

