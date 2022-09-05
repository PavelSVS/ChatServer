/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "Redis.h"

Redis::Redis(){
    onConnectCallback = NULL; 
    onSubscribeCallback = NULL; 
    onMessageCallback = NULL;
    onKeyCallback = NULL;
}
Redis::~Redis(){
    logMessage(LOGGER_DEBUG, "Redis::~Redis()");
}

int Redis::init(string addr, uint16_t port, string channel, string password, void* arg){
    this->chanel = channel;
    this->port = port;
    this->address = addr;
    this->aux = arg;
    this->password = password;
    logMessage(LOGGER_DEBUG, "Conecting...");
    rdClient.connect(addr, port, [this](const std::string &host, std::size_t port, cpp_redis::connect_state status){
        onConnect(host, port, status);
    },0 ,10, 10000); //timeout - 0ms, reconnections - 10000ms, reconnection retries - 10 times
    if (this->password.length() > 0){
        rdClient.auth(this->password, [](cpp_redis::reply& reply) {
        //! handle auth reply
        //! the callback is optional
        });
        //! send the auth command synchronously
        //! when sync_commit completes, the authentication will be done
        rdClient.sync_commit();
    }
    return 0;
}

void Redis::onConnect(const std::string &host, std::size_t port, cpp_redis::connect_state status){
    if (status == cpp_redis::connect_state::dropped){
            logMessage(LOGGER_DEBUG, "Redis client disconnected from %s:%d", host.c_str(), port);
        } else
    if (status == cpp_redis::connect_state::ok){
            logMessage(LOGGER_DEBUG, "Redis client connected to %s:%d", host.c_str(), port);
        if (this->onConnectCallback != NULL)
            onConnectCallback(this);
        subscribe();
    }
}

void Redis::subscribe(){
    rdSubscriber.auth(this->password, [](cpp_redis::reply& reply) {
        //! handle auth reply
        //! the callback is optional
        });
    rdSubscriber.connect(address, port, [this](const std::string& host, std::size_t port, cpp_redis::connect_state status){
        onSubscribe(host, port, status);
    });
}

void Redis::onSubscribe(const std::string& host, std::size_t port, cpp_redis::connect_state status){
        logMessage(LOGGER_DEBUG, "Redis::onSubscribe()");
        if (status == cpp_redis::connect_state::dropped) {
//            std::cout << "client disconnected from " << host << ":" << port << std::endl;
            logMessage(LOGGER_DEBUG, "Redis client subscription dropped from %s:%d", host.c_str(), port);            
        } else
        if (status == cpp_redis::connect_state::ok){
            if (onSubscribeCallback != NULL)
                onSubscribeCallback(this);
            if (this->chanel.find("*") != string::npos){
                logMessage(LOGGER_DEBUG, "Redis client (P) subscribed to \"%s\"", this->chanel.c_str());
                rdSubscriber.psubscribe(this->chanel, [this](const std::string& chan, const std::string& msg){
                    onMessage(chan, msg);
                });
            } else {
                logMessage(LOGGER_DEBUG, "Redis client subscribed to \"%s\"", this->chanel.c_str());
                rdSubscriber.subscribe(this->chanel, [this](const std::string& chan, const std::string& msg){
                    onMessage(chan, msg);
                });
            }
            
            logMessage(LOGGER_DEBUG, "Redis Connect state:OK");
            //rdSubscriber.psubscribe("*", [](const std::string& chan, const std::string& msg) {
            //    std::cout << "PMESSAGE " << chan << ": " << msg << std::endl;
            //});
            rdSubscriber.commit();
        } else
            logMessage(LOGGER_DEBUG, "Redis Connect state: Failed");
        
    }

void Redis::onMessage(const std::string& chan, const std::string& msg){
    if (this->onMessageCallback != NULL)
        this->onMessageCallback(chan, msg, this);
}
void Redis::publish(string channel, string message){
    this->rdClient.publish(channel, message);
    this->rdClient.commit();
}

void Redis::getAllArrayKeysByMask(string pattern){    
    
    rdClient.keys(pattern, [this](cpp_redis::reply &reply){
        if (!reply.is_error()){
            iterateReply(reply.as_array());
        }
    });
    rdClient.commit();
}
void Redis::iterateReply(vector<cpp_redis::reply> r){
    for (vector<cpp_redis::reply>::iterator itKey = r.begin(); itKey != r.end(); itKey++){
        this->getArray(itKey->as_string());
    }
}

void Redis::getKeyValue(string key){
    rdClient.get(key, [this](cpp_redis::reply &reply) {
        onKey(reply);
    });
    rdClient.commit();
}
void Redis::setKey(string key, string value){   
    rdClient.set(key, value, [this](cpp_redis::reply &reply){
        // report key added
    });
    rdClient.commit();
}
void Redis::hsetKey(string key, string field, string value){   
    rdClient.hset(key, field, value, [this](cpp_redis::reply &reply){
        // report key added
    });
    rdClient.commit();
}

void Redis::hsetKeyImmediate(string key, string field, string value){
    rdClient.hset(key, field, value);
    rdClient.commit();
}

string Redis::hgetKeyImmediate(string key, string field){    
    string ret = "";
    std::future<cpp_redis::reply> r = rdClient.hget(key, field);    
    rdClient.commit();
    r.wait();
    cpp_redis::reply cR = r.get();
    if (!cR.is_error() && cR.is_string())
        ret = cR.as_string();
    
    return ret;
}


void Redis::getArray(string key){
    rdClient.hgetall(key, [this, key](cpp_redis::reply &reply){
        onArray(reply, key);
    });
    rdClient.commit();    
}

void Redis::onKey(cpp_redis::reply &reply){
    if (this->onKeyCallback != NULL)
        this->onKeyCallback(reply, this);
}

void Redis::onArray(cpp_redis::reply &reply, string key){
    if (this->onArrayCallback != NULL)
        this->onArrayCallback(reply, key, this);
}

void Redis::setOnKeyCallback(std::function<void(cpp_redis::reply, void*)> onKeyCallback){
    this->onKeyCallback = onKeyCallback;
}

void Redis::setOnArrayCallback(std::function<void(cpp_redis::reply, string, void*)> onArrayCallback){
    this->onArrayCallback = onArrayCallback;
}

void Redis::setOnMessageCallback(std::function<void(string chan, string msg, void*)> onMessageCallback){
    this->onMessageCallback = onMessageCallback;
}

void Redis::setOnConnectCallback(std::function<void(void*)> onConnectCallback){
    this->onConnectCallback = onConnectCallback;
};

void Redis::setOnSubscribeCallback(std::function<void(void*)> onSubscribeCallback){
    this->onSubscribeCallback = onSubscribeCallback;
};
