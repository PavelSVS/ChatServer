#include <cstdlib>
#include "Chat.h"
#include "Logger.h"

using namespace std;

/*
 * 
 */
int main(int argc, char** argv) {
//    loggerLevel = LOGGER_INFO | LOGGER_WARNING | LOGGER_DEBUG | LOGGER_ERROR;
    loggerLevel = 0;
    bool typeFilter = false;
    std::string password = "";
    
    std::cout << "Cloudsgoods Websocket Chat Server v1.0" << std::endl;
/*    
    if (argc < 5) {
        std::cout << "Usage: "<<argv[0]<<" [--listen=xxxx] [--debug-level=N]" << std::endl;
        return 1;
    }
 */
    uint16_t port = 8888;
    if (argc >=6){
        int i;
        for (i = 5; i < argc; i++){
            string param;
            param.assign(argv[i]);
            if (param.find("--listen=")!=string::npos){
                param = param.substr(9);
                port = stoi(param);
            } else
            if (param.find("--debug-level=")!=string::npos){
            param = param.substr(14);
            int level = stoi(param);
            switch(level){
                case 0: loggerLevel = 0;
                cout << "debuglevel: 0" << endl;
                break;
                case 1: loggerLevel = LOGGER_ERROR | LOGGER_WARNING;
                cout << "debuglevel: 1" << endl;
                break;
                case 2: loggerLevel = LOGGER_ERROR | LOGGER_WARNING | LOGGER_INFO;
                cout << "debuglevel: 2" << endl;
                break;
                case 3: loggerLevel = LOGGER_ERROR | LOGGER_WARNING | LOGGER_INFO  | LOGGER_DEBUG;
                cout << "debuglevel: 3" << endl;
                break;
                default: loggerLevel = LOGGER_ERROR | LOGGER_WARNING | LOGGER_INFO  | LOGGER_DEBUG;
                cout << "debuglevel: 3(default)" << endl;
                break;
            }
        }
    }
    }
    
    loggerLevel = LOGGER_ERROR | LOGGER_WARNING | LOGGER_INFO  | LOGGER_DEBUG;    
    
    printf("Listening port: %d\n", port);
    Chat chat;
    chat.init(port);
    chat.run();
    std::cout << "Press Ctrl-C to quit" << std::endl;
    for (;;){
        usleep(10000);
    }
    return 0;
}

