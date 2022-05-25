/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WsClientData.h
 * Author: dmitry
 *
 * Created on April 26, 2021, 4:58 PM
 */

#ifndef WSCLIENTDATA_H
#define WSCLIENTDATA_H
#include <string>
#include <cstring>
#include <mutex>
#include <experimental/filesystem>
#include <chrono>
#include <unistd.h>
#include "Logger.h"

using namespace std;
using namespace std::chrono;
class WsClientData{
    public:
        int socket;
        int len;
        int hdrLen;
        uint16_t required;
        uint8_t maskOffset;
        uint8_t opcode;
        uint64_t pktNo;
        char* data;
        mutex dataMutex;
        std::string hexStr(uint8_t *data, int len);
        WsClientData(int socket);
        WsClientData(const WsClientData &s);
        ~WsClientData();
       
        void dropMessage(bool lock);
        int addBuffer(char* data, int size);
        void cleanBuffer(bool lock);

        void readHeader();
        
        string checkMessage();
        string onMessage();
    
};


#endif /* WSCLIENTDATA_H */

