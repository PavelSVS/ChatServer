/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BoostTCPSession.h
 * Author: dmitry
 *
 * Created on 26 мая 2021 г., 16:08
 */

#ifndef BOOSTTCPSESSION_H
#define BOOSTTCPSESSION_H
#include <boost/asio.hpp>
#include <optional>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <mutex>

#define READ_BUFFER_SIZE            64*1024

namespace io = boost::asio;
using tcp = io::ip::tcp;
using error_code = boost::system::error_code;
using namespace std::placeholders;
using message_handler = std::function<void (std::string, std::string, int)>;
using error_handler = std::function<void ()>;

struct SerializedBuffer {
    std::vector<char> _buf;
    size_t _pos = 0;

    size_t position() const { return _pos; }
    void position(size_t n) { _pos = n; }
    void rewind() { position(0); }
    char* bytes() { return _buf.data(); }

    size_t limit() { return _buf.size(); }
    void limit(size_t n) { _buf.resize(n); }

    SerializedBuffer(size_t n) : _buf(n) {}
};

class TCPSession : public std::enable_shared_from_this<TCPSession>{
    public:

        TCPSession(tcp::socket&& socket);
        ~TCPSession();
        void start(message_handler&& on_message, error_handler&& on_error);
        void post(std::string const& message);
        int getSocket();
        void close();
        std::string getAddress();
    private:
        tcp::socket socket;
        SerializedBuffer* buffer;
        std::mutex mutexRead;
        std::mutex mutexWrite;
        std::mutex mutexQueueModify;
        std::queue<std::string> outgoing;
        message_handler on_message;
        error_handler on_error;
        bool quit;
        
        void async_read();
        void on_read(boost::system::error_code error, std::size_t bytes_transferred);
        void async_write();

        void on_write(boost::system::error_code error, std::size_t bytes_transferred);
};



#endif /* BOOSTTCPSESSION_H */

