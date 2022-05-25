/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "Chat.h"

std::string randomString(const int len) {
    
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    srand( (unsigned) time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) 
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    
    
    return tmp_s;
    
}

Chat::Chat(){

    
}

Chat::~Chat(){

    
}

void Chat::init(uint16_t port){
    logMessage(LOGGER_DEBUG,"Initializing Websocket 0.0.0.0:%d..", port);
    server.setOnConnectedCallback(onWebsocketConnected);
    server.setOnMessageCallback(webSocketData);
    server.setOnCloseCallback(webSocketClose);
    
    this->port = port;
    this->listenInterface = "0.0.0.0";
}

void Chat::run(){
    logMessage(LOGGER_DEBUG,"Starting Websocket server..");
    server.init(this->port, this->listenInterface, this);
}
////////////////// REDIS //////////////////////////////

/*
 * When Redis control channel message reсeived
 * chan - channel name
 * msg - message
 * arg - points on us
 */


urlParts_t* Chat::splitUrl(string url)
{    
    int pos;
    urlParts_t *ret;
    ret = new urlParts_t;
    
    if ((pos = url.find("://")) != string::npos){
        ret->proto = url.substr(0,pos);
        url = url.substr(pos + 3);
    }
    
    if ((pos = url.find("/")) != string::npos){
        ret->host = url.substr(0, pos);
        url = url.substr(pos);
    } else 
    if ((pos = url.find("?")) != string::npos){
        ret->host = url.substr(0, pos);
        url = url.substr(pos+1);
    } else {
        ret->host = url;
        url = "";
    }
    
    if ((pos = url.find("?")) != string::npos){
        ret->path = url.substr(0,pos);
        url = url.substr(pos+1);
    } else
    if(url.length() > 0){
        ret->path = url;
        url = "";
    }
    
    if (url.length() > 0){
        ret->parameters = url;
        url = "";
    }
    
    return ret;
}

/*
 * Check user against database via API /api/check?id=<id>&x-session=<session>
 * API answer is 'true' or 'false'
 */
bool Chat::checkUser(string id, string session)
{
    return true;
}

/*
 * Just close raw socket. Shall not be done when client already added to hashmap
 */
void Chat::closeConnectionSocket(int socket)
{
    logMessage(LOGGER_DEBUG,"Disconnecting socket: %d\n", socket);
    this->server.close(socket);
}
/////////////////////////////////////////////////////////
/////////////// WEBSOCKET //////////////////////////////
void Chat::onWebsocketConnected(int socket, string params, char ip[], void* aux){
    Chat* self = (Chat*) aux;
    int pos;
    urlParts_t *urlParts = self->splitUrl(params);
    
    vector<string> parts;
    boost::split(parts, urlParts->parameters, boost::is_any_of("&"));  // split it by '&'
    string id="0";
    string session = "";
    for (vector<string>::iterator it=parts.begin(); it != parts.end(); it++){
        string item = *it;
        int begin = item.find("=");
        if (begin!=string::npos && begin < item.length()){
            string key = item.substr(0,begin);               // take left part
            string value = item.substr(begin+1);             // take right part
            if (key.compare("id") == 0){         // match to "id"
                // add a pair - id-socket
                id = value;
            } else
            if (key.compare("session") == 0){         // match to "id"
                session = value;
            }
        }
    }
    logMessage(LOGGER_DEBUG,"Connected client ID: %s session: %s\n", id.c_str(), session.c_str());
    
    if ((id.compare("0") != 0) && (session.length() > 0)){
        if (self->checkUser(id, session)){
            self->clientMutex.lock();
            struct ClientData data;
            data.id = id;
            data.socket = socket;
            data.path = urlParts->path.substr(1);
            self->clients[id] = data;
            self->clientMutex.unlock();

            return;
        }
    }
    
    self->closeConnectionSocket(socket);
}

void Chat::clearSent(bool lock){
    if (lock)
        this->clientMutex.lock();
    this->sentList.clear();
    if (lock)
        this->clientMutex.unlock();
}

bool Chat::isSent(string id, bool lock){
    bool res = false;
    if (lock)
        this->clientMutex.lock();
    res = (std::find(this->sentList.begin(), this->sentList.end(), id)) != this->sentList.end();
    if (lock)
        this->clientMutex.unlock();
    return res;
}
void Chat::sendMessageByID(std::string id, std::string message, bool lock){
    if (lock){
        this->clientMutex.lock();
    }
    if (!isSent(id, false)){
        for( const std::pair<std::string, ClientData>& n : this->clients ) {
            if ((n.first.compare(id) == 0)){
                this->server.send(message.data(), message.length(), n.second.socket, false);
                this->sentList.push_back(id);
            }
        }
    }
    if (lock){
        this->clientMutex.unlock();
    }
}

int Chat::getUserSocketByID(std::string id, bool lock){
    int ret = 0;
    if (lock)
        this->clientMutex.lock();
        for( const std::pair<std::string, ClientData>& n : this->clients ) {
            if ((n.first.compare(id) == 0)){
                ret = n.second.socket;
            }
        }
    if (lock)
        this->clientMutex.unlock();
    
    return ret;
}

ClientData* Chat::getUserBySocket(int socket, bool lock){
    ClientData* ret = nullptr;
    if (lock)
        this->clientMutex.lock();
        for( const std::pair<std::string, ClientData>& n : this->clients ) {
            if (n.second.socket == socket){
                ret = new ClientData;
                ret->id     = n.second.id;
                ret->path   = n.second.path;
                ret->socket = n.second.socket;
                
                break;
            }
        }
    if (lock)
        this->clientMutex.unlock();
    
    return ret;
}


void Chat::removeClientBySocket(int socket){
    this->clientMutex.lock();
    
    for( std::unordered_map<std::string, ClientData>::iterator it = this->clients.begin(); it !=  this->clients.end(); it++) {
        if (it->second.socket == socket){
            removeFromAllRooms(it->first);
            this->clients.erase(it);
            break;
        }
    }
    this->clientMutex.unlock();
}

void* Chat::webSocketClose(int socket, void* arg){
    Chat* self = (Chat*) arg;
    self->removeClientBySocket(socket);
    return NULL;
}

/*
 * Chat Message format: 
 * {"type":"broadcast/private/room", "id":"user_id/room_name", message":"message contents"}
 * Chat room create:
 * {"type":"mk_room","id":"room_name","password":"room_password"} // room name starting with @ is hidden
 * Join chat room:
 * {"type":"join_room","id":"room_name", "password":"room_password"}
 * Leave chat room:
 * {"type":"leave_room","id":"room_name"}
 * List chat rooms:
 * {"type":"list_rooms"}
 * List chat room participants:
 * {"type":"list_room","id":"room_name"}
 * List all users:
 * {"type":"list_all"}
 */

void Chat::webSocketData(int socket, void* buf, int nBytes, void* arg){
    Chat* self = (Chat*) arg;
    string cmd;
    string message;
    Json::FastWriter fastWriter;
    
    ClientData* client = self->getUserBySocket(socket);    
    
    if (!client){
        return;
    }
    
    cmd.assign((char*)buf,nBytes);
    command_t* command = self->parseCommand(cmd);
    
    command->root["from"] = client->id; // adding source ID to the message
    message = fastWriter.write(command->root); // build a new message with the source ID to not to be anonymous

    logMessage(LOGGER_DEBUG,"Command received: %s\n", command->type.c_str());
    
    if (command->type.compare("broadcast") == 0){ // message to all
        self->server.broadcast(message.data(), message.length(), false);
    } else
    if (command->type.compare("private") == 0){ // message to a user
        self->sendMessageByID(command->id, message, true);
        self->clearSent(true);
    } else
    if (command->type.compare("room") == 0){ // message to a user
        if (self->isInRoom(command->id, client->id)){
            self->sendMessageToRoom(command->id, message, true);
        }
    } else
    if (command->type.compare("mk_room") == 0){ // create room
        string roomName = command->id;
        //TODO: escape quotes for roomName
        if (self->createRoom(roomName, command->password, client->id)){
            self->sendMessageByID(client->id, "{\"success\":true,\"code\":0,\"message\":\"Room successfully created\",\"id\":\""+roomName+"\"}", true);
        } else {
            self->sendMessageByID(client->id, "{\"success\":false,\"code\":100,\"message\":\"Room can not be created\",\"id\":\""+roomName+"\"}", true);
        }
        self->clearSent(true);
    } else
    if (command->type.compare("join_room") == 0){ // message to a user
        string roomName = command->id;
        //TODO: escape quotes for roomName
        if (self->joinRoom(roomName, command->password, client->id) == 0){
            self->sendMessageByID(client->id, "{\"success\":true,\"code\":0,\"message\":\"Joined the room id:\""+roomName+"\"}", true);
        } else {
            self->sendMessageByID(client->id, "{\"success\":false,\"code\":101,\"message\":\"Can not join the room\",\"id\":\""+roomName+"\"}", true);
        }
        self->clearSent(true);
    } else
    if (command->type.compare("leave_room") == 0){ // message to a user
        string roomName = command->id;
        //TODO: escape quotes for roomName
        if (self->leaveRoom(roomName, client->id) == 0){
            self->sendMessageByID(client->id, "{\"success\":true,\"code\":0,\"message\":\"left the room\"\"id\":\""+roomName+"\"}", true);
        } else {
            self->sendMessageByID(client->id, "{\"success\":false,\"code\":101,\"message\":\"Can not leave the room\",\"id\":\""+roomName+"\"}", true);
        }
        self->clearSent(true);
    } else
    if (command->type.compare("list_rooms") == 0){ // list rooms
        std::vector<string>* roomList = self->listRooms();
        Json::Value jsonMap;
        
        int itemId = 0;
        for (std::vector<string>::iterator it = roomList->begin(); it != roomList->end(); ++it){
            jsonMap[std::to_string(itemId)] = *it;
            itemId++;
        }
        delete roomList;
        
        Json::Value answer;
        answer["success"] = true;
        answer["code"]    = 0;
        answer["message"] = "Rooms list retrieved";
        answer["data"]    = jsonMap;
        
        message = fastWriter.write(answer);
        self->sendMessageByID(client->id, message, true);
        self->clearSent(true);
    } else
    if (command->type.compare("list_room") == 0){ // list room participants
        logMessage(LOGGER_DEBUG,"LIST_ROOM\n");
        std::vector<string>* participantList = self->listRoom(command->id, client->id);
        if (participantList != nullptr){
            Json::Value jsonMap;

            int itemId = 0;
            for (std::vector<string>::iterator it = participantList->begin(); it != participantList->end(); ++it){
                jsonMap[std::to_string(itemId)] = *it;
                itemId++;
            }
            delete participantList;
            Json::Value answer;
            answer["success"] = true;
            answer["code"]    = 0;
            answer["message"] = "Room participants list retrieved";
            answer["data"]    = jsonMap;

            message = fastWriter.write(answer);
            self->sendMessageByID(client->id, message, true);
        } else {
            string roomName = command->id;
            //TODO: escape quotes for roomName
            self->sendMessageByID(client->id, "{\"success\":false,\"code\":101,\"message\":\"Can not retrieve list of the participants for room\",\"id\":\""+roomName+"\"}", true);        
        }
        self->clearSent(true);
    } else
    if (command->type.compare("list_all") == 0){ // list all users
        logMessage(LOGGER_DEBUG,"LIST_ALL\n");
        
        std::vector<string>* usersList = self->listAllUsers();
        Json::Value jsonMap;
        
        int itemId = 0;
        for (std::vector<string>::iterator it = usersList->begin(); it != usersList->end(); ++it){
            jsonMap[std::to_string(itemId)] = *it;
            itemId++;
        }
        delete usersList;
        Json::Value answer;
        answer["success"] = true;
        answer["code"]    = 0;
        answer["message"] = "Users list retrieved";
        answer["data"]    = jsonMap;
        
        message = fastWriter.write(answer);
        logMessage(LOGGER_DEBUG,"ClientID: %s\n", client->id.c_str());
        self->sendMessageByID(client->id, message, true);
        self->clearSent(true);
        
    }
        
    delete(command->reader); // clean up our memory
    delete(command);
}

std::vector<std::string>* Chat::listRoom(std::string roomName, std::string userId)
{
    std::vector<string>* ret = nullptr;
    room_t room;
    
    this->roomMutex.lock(); 
    try{
        room = this->rooms.at(roomName); 
        if (isInRoom(roomName, userId, false)){
            ret = new std::vector<string>;
            for( std::vector<std::string>::iterator it = room.participants.begin(); it !=  room.participants.end(); it++) {
                string name = *it;
                ret->push_back(name);
            }
        }
    } catch(const std::out_of_range& oor){
        // nothing
    }
    this->roomMutex.unlock();
    
    return ret;
}

std::vector<std::string>* Chat::listAllUsers()
{
    std::vector<string>* ret = new std::vector<string>;
    
    this->clientMutex.lock();    
    for( std::unordered_map<std::string, ClientData>::iterator it = this->clients.begin(); it !=  this->clients.end(); it++) {
        string name = it->first;
        ret->push_back(name);
    }    
    this->clientMutex.unlock();
    
    return ret;
}

void Chat::removeFromAllRooms(std::string userId)
{
   logMessage(LOGGER_DEBUG,"removeFromAllRooms(): %s\n", userId.c_str());
    
    this->roomMutex.lock();    
    for( std::unordered_map<std::string, room_t>::iterator it = this->rooms.begin(); it !=  this->rooms.end(); it++) {
        leaveRoom(it->first, userId, false);
    }    
    this->roomMutex.unlock();
   logMessage(LOGGER_DEBUG,"removeFromAllRooms(): done n");
    
}

std::vector<std::string>* Chat::listRooms()
{
    std::vector<string>* ret = new std::vector<string>;
    
    this->roomMutex.lock();    
    for( std::unordered_map<std::string, room_t>::iterator it = this->rooms.begin(); it !=  this->rooms.end(); it++) {
        string roomName = it->first;
        if (roomName.at(0) != '@'){ // do not list hidden rooms
            ret->push_back(roomName);
        }
    }    
    this->roomMutex.unlock();
    
    return ret;
}

int Chat::leaveRoom(std::string roomName, std::string userId, bool lock)
{
   logMessage(LOGGER_DEBUG,"leaveRoom(): %s\n", userId.c_str());
    room_t room;
    int ret = -1;
    if (lock){
        this->roomMutex.lock();
    }
    try{
        room = this->rooms.at(roomName); 
        if (isInRoom(roomName, userId, false)){
            room.participants.erase(std::remove(room.participants.begin(), room.participants.end(), userId), room.participants.end());            
            this->rooms[roomName] = room;
            this->sendMessageToRoom(roomName,"{\"type\":\"update\",\"code\":2,\"message\":\"User left the room\",\"id\":\""+userId+"\"}", false);
            ret = 0;
        }
        
    } catch(const std::out_of_range& oor){
   logMessage(LOGGER_DEBUG,"leaveRoom(): catch! \n");
        ret = -2;
    }    
    if (lock){
        this->roomMutex.unlock();
    }
   logMessage(LOGGER_DEBUG,"leaveRoom(): done\n");
    
    return ret;
    
}


int Chat::joinRoom(std::string roomName, std::string password, std::string userId)
{
    room_t room;
    int ret = -1;
    this->roomMutex.lock();    
    try{
        room = this->rooms.at(roomName); 
        if (room.password.compare(password) == 0){
            if (!isInRoom(roomName, userId, false)){
                room.participants.push_back(userId);
                this->rooms[roomName] = room;
                this->sendMessageToRoom(roomName,"{\"type\":\"update\",\"code\":1,\"message\":\"New user joined the room\",\"id\":\""+userId+"\"}", false);
                ret = 0;
            }
        }
        
    } catch(const std::out_of_range& oor){
        ret = -2;
    }    
    this->roomMutex.unlock();
    
    return ret;
    
}

bool Chat::createRoom(std::string roomName, std::string password, std::string userId)
{
    room_t room;
    bool ret = false;
    this->roomMutex.lock();    
    try{
        room = this->rooms.at(roomName);        
    } catch(const std::out_of_range& oor){
        room.name = roomName;
        room.password = password;
        room.participants.push_back(userId);
        this->rooms[roomName] = room;
        ret = true;
    }    
    this->roomMutex.unlock();
    
    return ret;
}

void Chat::sendMessageToRoom(std::string roomName, std::string message, bool lock){
    room_t room;

    if (lock){
        this->roomMutex.lock();    
    }
    try{
        room = this->rooms.at(roomName);
        for(auto it = room.participants.begin(); it != room.participants.end(); ++it) {
            sendMessageByID(*it, message, false);
        }
        this->clearSent(false);
    } catch(const std::out_of_range& oor){
        // nothing
    }
    
    if (lock){
        this->roomMutex.unlock();
    }
}

bool Chat::isInRoom(string roomName, string id, bool lock)
{
    bool ret = false;
    room_t room;
    if (lock){
        this->roomMutex.lock();
    }
    try{
        room = this->rooms.at(roomName);
        if ( std::find(room.participants.begin(), room.participants.end(), id) != room.participants.end() ){
            ret = true;
        }
    } catch(const std::out_of_range& oor){
        // nothing
    }
    if (lock){
        this->roomMutex.unlock();
    }
    
    return ret;
}

command_t* Chat::parseCommand(string command)
{
    command_t* ret = new command_t;
    ret->reader = ret->builder.newCharReader();
    /*
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader * reader = builder.newCharReader();
    Json::FastWriter fastWriter;
     */
    string errors;

    try{
        ret->reader->parse(command.c_str(), command.c_str() + command.size(), &ret->root, &errors); // разбираем сообщение на поля
//        Json::CharReader * reader = builder.newCharReader();

        if (ret->root.isMember("type")){
            const Json::Value type = ret->root["type"];
            ret->type = type.asString();
        }
        if (ret->root.isMember("id")){
            const Json::Value id = ret->root["id"];
            ret->id = id.asString();
        }
        if (ret->root.isMember("message")){
            const Json::Value message = ret->root["message"];
            ret->message = message.asString();
        }
        if (ret->root.isMember("password")){
            const Json::Value password = ret->root["password"];
            ret->password = password.asString();
        }
    } catch (...){
    }
    
    return ret;
    
}

