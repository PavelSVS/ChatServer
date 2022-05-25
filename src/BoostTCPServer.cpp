/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "BoostTCPServer.h"
BoostTCPServer::BoostTCPServer(io::io_service& io_context, std::uint16_t port) : io_context(io_context)
    , acceptor  (io_context, tcp::endpoint(tcp::v4(), port)){
    this->data_handler_callback = NULL;
    this->on_close_callback = NULL;
}


void BoostTCPServer::async_accept(){
    logMessage(LOGGER_DEBUG, "BoostTCPServer::async_accept()");
    clientsMutex.lock();
    socket.emplace(io_context);
    clientsMutex.unlock();
    
    acceptor.async_accept(*socket, [&] (error_code error){
        clientsMutex.lock();
        auto client = std::make_shared<TCPSession>(std::move(*socket));

        clients.insert(client);
        clientsMutex.unlock();

        client->start(
            std::bind(&BoostTCPServer::onMessage, this, _1, _2, _3),
            [&, weak = std::weak_ptr(client), sock = client->getSocket()]{
                
                if(auto shared = weak.lock(); shared && clients.erase(shared)){
                    this->onClose(sock);
                }
            }
        );

        async_accept();
    });
}

void BoostTCPServer::onClose(int socket){
    if (this->on_close_callback)
        this->on_close_callback(socket, this->aux);
}


void BoostTCPServer::onMessage(std::string const& message, std::string const& ip, int const& socket){
    if (this->data_handler_callback)
        this->data_handler_callback(socket, (void*)message.data(), message.length(), (char*)ip.data(), this->aux);

}

int BoostTCPServer::send(char* data, int len, int socket){
    std::string message;
    message.assign(data, len);
    return this->send(message, socket);
}

int BoostTCPServer::send(std::string s, int socket){
    clientsMutex.lock();
    for(auto& client : clients){
        if (client->getSocket() == socket){
            client->post(s);
            break;
        }
    }
    clientsMutex.unlock();
    return 0;
}

int BoostTCPServer::broadcast(std::string s){
    clientsMutex.lock();
    for(auto& client : clients){
        client->post(s);
    }
    clientsMutex.unlock();
    return 0;
}


void BoostTCPServer::forceCloseSocket(int socket){
    clientsMutex.lock();
    for(auto& client : clients){
        if (client->getSocket() == socket){
            client->close();
            break;
        }
    }
    clientsMutex.unlock();
    
}

int BoostTCPServer::init(void* arg){
    this->aux = arg;
    pthread_create( &Listener , NULL ,  Listener_process ,  (void*)this);
    
    return 0;
}

void* BoostTCPServer::Listener_process(void *arg){
    BoostTCPServer* self = (BoostTCPServer*) arg;
 
    self->async_accept();
    self->io_context.run();
    
    return NULL;
}

void BoostTCPServer::setDataHandlerCallback(std::function<void*(int, void*, int, char[], void*)> data_handler_callback){
    this->data_handler_callback = data_handler_callback;
};

void BoostTCPServer::setOnCloseCallback(std::function<void(int, void*)> on_close_callback){
    this->on_close_callback = on_close_callback;
};
