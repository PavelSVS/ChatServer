/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "WsClientData.h"

WsClientData::WsClientData(int socket){    // Constructor
    this->socket = socket; 
    this->len = 0; 
    this->required = 0; 
    this->data = NULL;
    this->maskOffset = 0;
    this->hdrLen = 0;
    this->pktNo = 0;
}

WsClientData::WsClientData(const WsClientData &s): dataMutex() { // copy constructor
    this->socket = s.socket;
    this->len = s.len;
    this->required = s.required;
    this->data = s.data;
    this->maskOffset = s.maskOffset;
    this->hdrLen = s.hdrLen;
    this->pktNo = s.pktNo;
}

WsClientData::~WsClientData(){
    dataMutex.lock();
    if ((this->len > 0) && (this->data != NULL))
        try{
        free(this->data);
        }catch(...){
        }
    this->socket = -1;
    this->len = 0;
    this->required = 0;
    dataMutex.unlock();
}

void WsClientData::dropMessage(bool lock){
    if (lock)
        dataMutex.lock();
    int remainder = len - (hdrLen + required);
    if (remainder > 0){
        char* newData = (char*)malloc(remainder);
        memcpy(newData, data + hdrLen + required, remainder);
        free(data);
        data = newData;
        len = remainder;
        readHeader();
    } else
        cleanBuffer(false);
    if (lock)
        dataMutex.unlock();
}

int WsClientData::addBuffer(char* data, int size){
    dataMutex.lock();
    if (this->len == 0){
        this->data = (char*)malloc(size);
        if (this->data == NULL)
            return -2;  // no memory
        memcpy(this->data, data, size);
        this->len = size;
        readHeader();
    } else {
        char* resized;
        resized = (char*)realloc(this->data, this->len + size);
        if (resized == NULL)
            return -2; // no memory
        this->data = resized;
        memcpy(this->data+this->len, data, size);
        this->len += size;
    }
    dataMutex.unlock();
    return 0;
}

void WsClientData::cleanBuffer(bool lock){
    if (lock)
        dataMutex.lock();
    if ((this->len > 0) && (this->data != NULL)){
        try{
            free(this->data);
        }catch(...){
        }
    }
    this->data = NULL;
    this->len = 0;
    this->required = 0;
    this->hdrLen = 0;
    this->maskOffset = 0;            
    if (lock)
        dataMutex.unlock();
}

void WsClientData::readHeader(){
    //logMessage(LOGGER_DEBUG, "WebsocketServer::readHeader()");
    if (this->len < 3)
        return;
    this->opcode = (uint8_t)this->data[0] & 0x0F;

    uint16_t frameSizeCode = (uint8_t)this->data[1] & 0x7F;
    //logMessage(LOGGER_DEBUG, "WebsocketServer::readHeader(): opcode: %2x FSC: %2x", opcode, frameSizeCode);

    if (frameSizeCode < 126){ // short message
        maskOffset = 0;
    } else
    if (frameSizeCode == 126){ // 126 < length < 65535
        frameSizeCode = 0;
        frameSizeCode = (uint8_t)this->data[2] << 8;
        frameSizeCode += (uint8_t)this->data[3];
        this->maskOffset = 2;
        //logMessage(LOGGER_DEBUG, "WebsocketServer::readHeader(): msg length < 65535; data[2]: %2x data[3]]: %2x length: %4x", (uint8_t)data[2], (uint8_t)data[3], frameSizeCode);
    } else {
        //TODO: request for disconnect !!!!!
    }
    this->hdrLen = 6 + this->maskOffset;
    this->required = frameSizeCode;
}

string WsClientData::checkMessage(){
    string ret = "";
    dataMutex.lock();
    if ((this->len > 0) && ((this->len - this->hdrLen) >= this->required)){
        ret = onMessage();
        dropMessage(false);
    }
    dataMutex.unlock();        

    return ret;
}

string WsClientData::onMessage(){
    uint8_t mask[4];
    string request;

    mask[0] = this->data[2+maskOffset];
    mask[1] = this->data[3+maskOffset];
    mask[2] = this->data[4+maskOffset];
    mask[3] = this->data[5+maskOffset];
    char* message = (char*) malloc(this->required);
    this->pktNo++;
    for (int i=0; i < this->required; i++)
        message[i] = this->data[i+6+this->maskOffset] ^ mask[i % 4]; // apply mask to the message
    request.assign(message, this->required);
    //logMessage(LOGGER_DEBUG, "WebsocketServer::Client::onMessage()-> required: %d, actual: %d", this->required, request.length());
    free (message);

    return request;
}
