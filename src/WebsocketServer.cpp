/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WebsocketServer.h"

WebsocketServer::WebsocketServer(){
    this->listenPort = 0;
    this->listenInterface = "";
    this->on_connected_callback = NULL;
    this->on_close_callback = NULL;
    this->on_message_callback = NULL;
}

WebsocketServer::~WebsocketServer(){
    delete server;
}

int WebsocketServer::init(uint16_t port, string interface, void* arg){

    this->listenPort = port;
    this->listenInterface = interface;
    this->aux = arg;
#ifdef BOOST_TCP_SERVER    
    server = new BoostTCPServer(this->io_context, port);
#else
    server = new TCPServer();
#endif
    server->setDataHandlerCallback(socketData);
    server->setOnCloseCallback(socketClose);
#ifdef BOOST_TCP_SERVER
    server->init(this);
#else
    server->init(interface, port, this);
#endif
    
    return 0;
}

WsClientData* WebsocketServer::newBuffer(int socket){
    WsClientData* ret = NULL;
    //logMessage(LOGGER_DEBUG, "WebsocketServer::newBuffer(%d)", socket);
    
    if (getBuffer(socket) == NULL){
        tempDataMutex.lock();
        tempData.push_back(WsClientData(socket));
        tempDataMutex.unlock();
        ret = getBuffer(socket);
    }
    return ret;
}

std::optional<std::list<WsClientData>::iterator> WebsocketServer::getBufferIterator(int socket){
    for (std::list<WsClientData>::iterator i=tempData.begin(); i!=tempData.end();i++){
        if (i->socket == socket){
            return i;
        }
    }
        return {};
}

WsClientData* WebsocketServer::getBuffer(int socket){
    WsClientData* ret = NULL;
    //logMessage(LOGGER_DEBUG, "WebsocketServer::getBuffer(%d)", socket);
    tempDataMutex.lock();
    auto i = getBufferIterator(socket);
    if (i != std::nullopt){
        ret = &(*(*i));
    } else {
        ret = NULL;
        //logMessage(LOGGER_DEBUG, "WebsocketServer::getBuffer(%d) is NULL", socket);
    }
    tempDataMutex.unlock();
    return ret;

}
void WebsocketServer::deleteBuffer(int socket){
    //logMessage(LOGGER_DEBUG, "WebsocketServer::deleteBuffer(%d)", socket);
    tempDataMutex.lock();
    auto i = getBufferIterator(socket);
    if (i != std::nullopt){
        tempData.erase(*i);
    }
    
    tempDataMutex.unlock();
    
}

void WebsocketServer::cleanBuffer(int socket){
    //logMessage(LOGGER_DEBUG, "WebsocketServer::cleanBuffer(%d)", socket);
    tempDataMutex.lock();
    auto i = getBufferIterator(socket);
    if (i != std::nullopt){
        (*i)->cleanBuffer(true);
    }
    tempDataMutex.unlock();
    
}



int WebsocketServer::addToBuffer(int socket, char* data, int len){
    WsClientData* buf = NULL;
    //logMessage(LOGGER_DEBUG, "WebsocketServer::addToBuffer(%d)", socket);
    if ((buf = getBuffer(socket)) != NULL){
        tempDataMutex.lock();
        buf->addBuffer(data, len);
        tempDataMutex.unlock();
    } else
        return -1;    // no associated buffer
    
    return 0;
}

string WebsocketServer::getParameters(string req){
    int begin, end;
    string request = req;
    string ret = "";
    //logMessage(LOGGER_DEBUG, "WebsocketServer::getParameters() req->%s", req.c_str());
    begin = request.find("GET /");
    if (begin != string::npos){
        request = request.substr(begin+4);
        begin = 0;
        end = request.find(" ");
        if (end == string::npos){
            return ret;
        }
        ret = request.substr(begin, end);
    }
    //logMessage(LOGGER_DEBUG, "WebsocketServer::getParameters() ret->%s", ret.c_str());
    return ret;
}

string WebsocketServer::buildInitAnswer(string request){
    string ret = "HTTP/1.1 400 Wrong 'Sec-WebSocket-Key' contents\r\n\r\n";
    string b64, key;
    int begin, end;
    //logMessage(LOGGER_DEBUG, "buildInitAnswer()");
    begin = request.find("Sec-WebSocket-Key:");
    if (begin != string::npos){
        begin += 19; // length of "Sec-WebSocket-Key: "
        end = request.find("\r\n", begin);
        if (end != string::npos){
            string key = request.substr(begin, end-begin);
            key = key + GUID;
            //logMessage(LOGGER_DEBUG, "buildInitAnswer() key->%s", key.c_str());
            unsigned char keySHA1[128];
            memcpy(keySHA1, key.c_str(), key.length());
            unsigned char hash[SHA_DIGEST_LENGTH];
            SHA1(keySHA1, key.length(), hash);
            b64 = base64_encode(hash, SHA_DIGEST_LENGTH, false);
            ret = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+b64+"\r\n\r\n";
        } 
    }
    //logMessage(LOGGER_DEBUG, "buildInitAnswer() ret->%s", ret.c_str());
    return ret;
}


void* WebsocketServer::socketData(int socket, void* buf, int nBytes, char ip[], void* arg){
    string request;
    string params;
    WsClientData* client;
    WebsocketServer* self = (WebsocketServer*) arg;
    
    
    request.assign((char*)buf, nBytes);
    if ((client = self->getBuffer(socket)) == NULL){ // check if client exists
        params = self->getParameters(request);       // if not, get request parameters
    
        if (params.length() > 0){ // GET found
            //logMessage(LOGGER_DEBUG, "WebSocket %d params: %s", socket, params.c_str());
            request = self->buildInitAnswer(request);
            self->server->send(request.data(), request.length(),socket);
            if (request.length() < 60) // if wrong request, and we answer 400 error code
                self->server->forceCloseSocket(socket);
            else                       // all OK
                self->onConnected(socket, params, ip);
        } else { // if no GET found, quit
            logMessage(LOGGER_DEBUG, "WebSocket %d Bad parameters", socket);
            request = "HTTP/1.1 400 Bad parameters\r\n\r\n";
            self->server->send(request.data(), request.length(),socket);
            self->server->forceCloseSocket(socket);
        }
    } else {
        client->addBuffer((char*)buf, nBytes);
        while (self->parseWebsocketBuffer(client, ip)){
            
        }

    }
    return NULL;
}

bool WebsocketServer::parseWebsocketBuffer(WsClientData* client, char ip[]){
    bool ret = false;
    
    
    if ((client->len > 0) && (client->len >= client->required)){
        ret = true;
        if (client->opcode == 9){
            string pong = "PONG";
            
            client->dropMessage(true);
            sendInternal(pong.data(), pong.length(),false, true);        
            
        } else 
        if (client->opcode == 8){ // Close
            client->dropMessage(true);
            server->forceCloseSocket(client->socket);
        } else 
        if (client->opcode == 1){
            
            string message;
            if ((message = client->checkMessage()) != "" ){
                if (this->on_message_callback != NULL){
                    try{
                        this->on_message_callback(client->socket, message.data(), message.length(), aux);
                    }catch(...){
                    } 

                }
            }
        } else {
            logMessage(LOGGER_DEBUG, "WebsocketServer::parseWebsocketBuffer(): dropping message");
            client->dropMessage(true);
        }
    }
    return ret;
}

void WebsocketServer::onConnected(int socket, string params, char ip[]){
    //logMessage(LOGGER_DEBUG, "WebsocketServer::onConnected(%d)", socket);
    if (this->on_connected_callback != NULL)
        try{
            this->on_connected_callback(socket, params, ip, aux);
        }catch(...){
        }
    //logMessage(LOGGER_DEBUG, "WebsocketServer::onConnected(%d) quit", socket);
    newBuffer(socket);
}

void* WebsocketServer::socketClose(int socket, void* arg){
    //logMessage(LOGGER_DEBUG, "WebsocketServer::socketClose(%d)", socket);
    WebsocketServer* self = (WebsocketServer*) arg;
    if (self->on_close_callback != NULL)
        try{
            self->on_close_callback(socket, self->aux);
        }catch(...){
        }
    self->deleteBuffer(socket);
    return NULL;
}

void WebsocketServer::close(int socket){
    server->forceCloseSocket(socket);
}

/*
 * send websocket encoded data
 * data - data to send
 * len - data length to send
 * socket - socket to send
 * binary - flag to indicate whether data text or binary
 */
void WebsocketServer::send( char* data, int len, int socket, bool binary){
    sendInternal(data, len, socket, binary, false);
}

void WebsocketServer::sendInternal( char* data, int len, int socket, bool binary, bool pong){
    int headerLen;
    if (len <= 0)
        return;
    
    if (len < 126) // less than 125 bytes
        headerLen = 2;
    else
    if (len < 65536)
        headerLen = 4;
    else
        headerLen = 10;
    
    char* buffer = (char*) malloc(len+headerLen);
    if (pong)
        buffer[0] = 0x0A; // binary data
    if (binary)
        buffer[0] = 0x82; // binary data
    else
        buffer[0] = 0x81; // text data

    if (len < 126)
        buffer[1] = len & 0xFF;
    else 
    if (len < 65536){
        buffer[1] = 126; // more than 125 bytes
        buffer[2] = (len >> 8) & 0xFF;
        buffer[3] = len & 0xFF;
    } else {
        buffer[1] = 127; // more than 65535 bytes

        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;

        buffer[6] = (len >> 24) & 0xFF;
        buffer[7] = (len >> 16) & 0xFF;
        buffer[8] = (len >> 8) & 0xFF;
        buffer[9] = len & 0xFF;
    }
    memcpy(buffer+headerLen, data, len);
    
    server->send(buffer, len+headerLen, socket);
    free(buffer);
}

void WebsocketServer::broadcast( char* data, int len, bool binary){
    broadcastInternal(data, len, binary, false);
}
void WebsocketServer::broadcastInternal( char* data, int len, bool binary, bool pong){
    int headerLen;
    if (len <= 0)
        return;
    
    if (len < 126) // less than 125 bytes
        headerLen = 2;
    else
    if (len < 65536)
        headerLen = 4;
    else
        headerLen = 10;
    
    char* buffer = (char*) malloc(len+headerLen);
    if (pong)
        buffer[0] = 0x0A; // binary data
    if (binary)
        buffer[0] = 0x82; // binary data
    else
        buffer[0] = 0x81; // text data

    if (len < 126)
        buffer[1] = len & 0xFF;
    else 
    if (len < 65536){
        buffer[1] = 126; // more than 125 bytes
        buffer[2] = (len >> 8) & 0xFF;
        buffer[3] = len & 0xFF;
    } else {
        buffer[1] = 127; // more than 65535 bytes

        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;

        buffer[6] = (len >> 24) & 0xFF;
        buffer[7] = (len >> 16) & 0xFF;
        buffer[8] = (len >> 8) & 0xFF;
        buffer[9] = len & 0xFF;
    }
    memcpy(buffer+headerLen, data, len);
    string message;
    message.assign(buffer, len+headerLen);
    //logMessage(LOGGER_DEBUG, "WebsocketServer::broadcast(): %s", message.c_str());
    server->broadcast(message);    
    free(buffer);
}

void WebsocketServer::setOnConnectedCallback(std::function<void(int, string, char[], void*)>on_connected_callback){
    this->on_connected_callback = on_connected_callback;
}

void WebsocketServer::setOnMessageCallback(std::function<void(int, void*, int, void*)>on_message_callback){
    this->on_message_callback = on_message_callback;
}

void WebsocketServer::setOnCloseCallback(std::function<void(int, void*)>on_close_callback){
    this->on_close_callback = on_close_callback;
}

std::string WsClientData::hexStr(uint8_t *data, int len)
{
     std::stringstream ss;
     ss << std::hex;

     for( int i(0) ; i < len; ++i )
         ss << std::setw(2) << std::setfill('0') << (int)data[i] << " ";

     return ss.str();
}