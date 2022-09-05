/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Bridge.h
 * Author: dmitry
 *
 * Created on June 22, 2021, 3:23 PM
 */

#ifndef CHAT_H
#define CHAT_H
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <mutex>
#include <jsoncpp/json/json.h>

#include "WebsocketServer.h"
#include "Redis.h"
#include "Logger.h"

using namespace std;

struct ClientData{
    int socket;
    string id;
    string path;
};

struct urlParts_t{
    string proto;
    string host;
    string path;
    string parameters;
};

struct command_t{
    string type;
    string id;
    string message;
    string password;
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader * reader;    
};

struct room_t{
    string name;
    std::vector <string> participants;
    string password;
};

class Chat{

    public:
        Chat();
        ~Chat();
        void init(uint16_t port);
        void run();
    protected:
        Redis redis;
        string redisHost;
        uint16_t redisPort;
        string redisChannel;
        string redisPassword;
        
        WebsocketServer server;
        uint16_t port;
        string listenInterface;
        std::unordered_map<std::string, ClientData> clients;
        std::unordered_map<std::string, room_t> rooms;
        std::mutex clientMutex;
        std::mutex roomMutex;
        std::vector <string> sentList;
        
        urlParts_t* splitUrl(string url);
        
        bool isSent(string id, bool lock=false);
        void sendMessageByRole(std::string roles, std::string message, bool lock=false);
        void removeClientBySocket(int socket);
        void sendMessageByID(std::string id, std::string message, bool lock=false);
        std::vector<std::string>* listAllUsers();
        int getUserSocketByID(std::string id, bool lock = true);
        ClientData* getUserBySocket(int socket, bool lock = true);

        //========ROOM Handling routines =======================================
        bool isInRoom(string roomName, string id, bool lock = true);
        void sendMessageToRoom(std::string id, std::string message, bool lock=true);
        bool createRoom(std::string roomName, std::string password, std::string userId);
        int joinRoom(std::string roomName, std::string password, std::string userId);
        int leaveRoom(std::string roomName, std::string userId, bool lock = true);
        std::vector<std::string>* listRooms();
        std::vector<std::string>* listRoom(std::string roomName, std::string userId);
        void removeFromAllRooms(std::string userId);
        //======== END OF ROOM Handling routines ===============================
        
        void clearSent(bool lock=false);
        void closeConnectionSocket(int socket);
        bool checkUser(string id, string session);
        command_t* parseCommand(string command);
        static void webSocketData(int socket, void* buf, int nBytes, void* arg);
        static void* webSocketClose(int socket, void* arg);
        static void onWebsocketConnected(int socket, string params, char ip[], void* aux);

        static void onConnectRedis(void* arg);
        static void onMessageRedis(string chan, string msg, void* arg);
        static void onSubscribeRedis(void* arg);

};

#endif /* CHAT_H */

