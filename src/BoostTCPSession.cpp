/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "BoostTCPSession.h"

TCPSession::TCPSession(tcp::socket&& socket) : socket(std::move(socket)) {
    buffer = nullptr;
}

TCPSession::~TCPSession(){
    mutexRead.lock();
    delete buffer;
    mutexRead.unlock();
}

void TCPSession::start(message_handler&& on_message, error_handler&& on_error){
    this->on_message = std::move(on_message);
    this->on_error = std::move(on_error);
    async_read();
}

void TCPSession::post(std::string const& message){
    mutexQueueModify.lock();
    bool idle = outgoing.empty();
    outgoing.push(message);
    mutexQueueModify.unlock();
    if(idle){
        async_write();
    }
}

void TCPSession::async_read(){
       delete buffer; // getting rid of old data 
       buffer = new SerializedBuffer((uint) READ_BUFFER_SIZE);
       buffer->position(0);
    
        io::async_read(socket, io::buffer(buffer->bytes(), READ_BUFFER_SIZE),
                io::transfer_at_least(1),
                std::bind(&TCPSession::on_read, shared_from_this(), _1, _2)
               
                );
}

void TCPSession::on_read(error_code error, std::size_t bytes_transferred){
    if(!error){
        mutexRead.lock();
        std::stringstream message;
        buffer->rewind();
        uint limit = (uint) bytes_transferred;
        buffer->limit(limit);
        std::string s;
        s.assign(buffer->bytes(), limit);
        message << s;
        on_message(message.str(), getAddress(), getSocket());
        mutexRead.unlock();
        async_read();
    } else {
        socket.close(error);
        on_error();
    }
}

void TCPSession::async_write(){
    /*
    std::string message;
    message = outgoing.front();
    io::async_write(socket, io::buffer(message.data(), message.length()), 
            std::bind(&TCPSession::on_write, shared_from_this(), _1, _2));
    */
    this->mutexWrite.lock();
    io::async_write(socket, io::buffer(outgoing.front()), 
            std::bind(&TCPSession::on_write, shared_from_this(), _1, _2));
//    this->mutexWrite.unlock();
    
}

void TCPSession::on_write(error_code error, std::size_t bytes_transferred){
    bool empty;
    this->mutexWrite.unlock();

    if(!error){
        mutexQueueModify.lock();
        outgoing.pop();
        empty = outgoing.empty();
        mutexQueueModify.unlock();
        if(!empty){
            async_write();
        }
    } else {
        socket.close(error);
        on_error();
    }
}

int TCPSession::getSocket(){ 
    return socket.native_handle();
}

std::string TCPSession::getAddress(){
    return socket.remote_endpoint().address().to_string();
}

void TCPSession::close(){ 
    error_code error;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    socket.close(error);    
}
