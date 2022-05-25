/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AsioWSServer.h
 * Author: dmitry
 *
 * Created on 28 сентября 2021 г., 18:12
 */

#ifndef ASIOWSSERVER_H
#define ASIOWSSERVER_H
#include "asio_service.h"
#include "server/ws/ws_server.h"

class ChatSession : public CppServer::WS::WSSession
{
public:
    using CppServer::WS::WSSession::WSSession;

protected:
    void onWSConnected(const CppServer::HTTP::HTTPRequest& request) override;

    void onWSDisconnected() override;

    void onWSReceived(const void* buffer, size_t size) override;

    void onError(int error, const std::string& category, const std::string& message) override;
};

class ChatServer : public CppServer::WS::WSServer
{
public:
    using CppServer::WS::WSServer::WSServer;

protected:
    std::shared_ptr<CppServer::Asio::TCPSession> CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer>& server) override;

protected:
    void onError(int error, const std::string& category, const std::string& message) override;
};



#endif /* ASIOWSSERVER_H */

