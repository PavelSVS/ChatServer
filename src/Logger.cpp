#include "Logger.h"
 int loggerDevice = LOGGER_DEVICE_CONSOLE;
 int loggerLevel = 1;
 
int logMessage(int level, const char* fmt, ...){
    va_list args;
    auto time = std::time(nullptr);
    char buffer[MAX_LOG_BUFFER_LENGTH];
    
    if ((level & loggerLevel) == 0)
        return 0;

    va_start(args, fmt);
    int rc = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
 
    if (loggerDevice & LOGGER_DEVICE_CONSOLE)
        std::cout << std::put_time(std::gmtime(&time), "%F %T") << "|" << std::string(buffer) << std::endl;
    
    if (loggerDevice & LOGGER_DEVICE_SYSLOG){
        int sysLogLevel = 0;
        if (level & LOGGER_INFO)
            sysLogLevel |= LOG_INFO;
        if (level & LOGGER_DEBUG)
            sysLogLevel |= LOG_DEBUG;
        if (level & LOGGER_WARNING)
            sysLogLevel |= LOG_WARNING;
        if (level & LOGGER_ERROR)
            sysLogLevel |= LOG_ERR;
        
        syslog(sysLogLevel, "%s\n", &buffer);
        
    }
    
    
    return rc;

}