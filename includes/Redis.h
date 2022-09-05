/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Redis.h
 * Author: dmitry
 *
 * Created on 24 ноября 2020 г., 8:47
 */

#ifndef REDIS_H
#define REDIS_H
#include <cpp_redis/cpp_redis>
#include <cpp_redis/misc/macro.hpp>
#include <string>
#include <functional>
#include <future>
#include "Logger.h"

using namespace std;

class Redis{
    public:
        void* aux;
        
        Redis();
        ~Redis();
        int init(string addr, uint16_t port, string channel, string password, void* arg);
        void setOnConnectCallback(std::function<void(void*)> onConnetCallback);
        void setOnSubscribeCallback(std::function<void(void*)> onSubscribeCallback);
        void setOnMessageCallback(std::function<void(string chan, string msg, void*)> onMessageCallback);
        void setOnKeyCallback(std::function<void(cpp_redis::reply, void*)> onKeyCallback);
        void setOnArrayCallback(std::function<void(cpp_redis::reply, string, void*)> onArrayCallback);
        void getKeyValue(string key);
        void setKey(string key, string value);
        void hsetKey(string key, string field, string value);
        void hsetKeyImmediate(string key, string field, string value);
        string hgetKeyImmediate(string key, string field);
        void getArray(string key);
        void getAllArrayKeysByMask(string pattern);
        void publish(string channel, string message);
    protected:
        void subscribe();
        void iterateReply(vector<cpp_redis::reply> r);
        void onConnect(const std::string &host, std::size_t port, cpp_redis::connect_state status);
        void onSubscribe(const std::string& host, std::size_t port, cpp_redis::connect_state status);
        void onMessage(const std::string& chan, const std::string& msg);
        void onKey(cpp_redis::reply &reply);        
        void onArray(cpp_redis::reply &reply, string key);
        string address;
        string password;
        uint16_t port;
        string chanel;
        cpp_redis::client rdClient;
        cpp_redis::subscriber rdSubscriber;
        std::function<void(void*)> onSubscribeCallback;
        std::function<void(void*)> onConnectCallback;
        std::function<void(cpp_redis::reply, void*)> onKeyCallback;
        std::function<void(cpp_redis::reply, string, void*)> onArrayCallback;
        std::function<void(string, string , void*)> onMessageCallback;

};


#endif /* REDIS_H */

