#include <ctime>
#include <cstdarg>
#include <arpa/inet.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <mutex>

#include "../include/Log.h"

static std::string getCurrentDateTime();
static std::string getCurrentTime();

Log::Level Log::logLevel = Log::Level::ERROR;

namespace Log {
    static std::mutex mutexLog;
    static std::mutex mutexLogW3c;
    static FILE *logFile = fopen("log.txt", "a");
}

/**
 * @desc used to set and store client IP
 * addr untill it will be written to the 
 * log file in W3C format
 * @params c-string clIP - client ip to store
 * @return pointer to static string with client IP
*/
const char *Log::setClientIp(const char *clIP = nullptr) {
    static char clientIP[INET6_ADDRSTRLEN] = { 0 };

    if(clIP && logLevel != Level::NONE)
        strcpy(clientIP, clIP);
    return clientIP;
}

/**
 * @desc it's printf-like function, that supports
 * only two format specifiers - %i (int), %s (char *).
 * Writes formated output into file.
 * @params first arg is format string, that defines
 * the way how the resulted string will look like 
 * @return void
*/
void Log::writeLog(Log::Level lvl, const char *firstArg, ...) {
    va_list args;

    if(!logFile || logLevel < lvl)
        return ;
    
    mutexLog.lock();
    va_start(args, firstArg);
    std::string dateTime = getCurrentDateTime();
    // write log-level
    if(lvl == Level::ERROR)
        fprintf(logFile, "\\>\t[ERROR]\n");
    else if(lvl == Log::Level::DEBUG)
        fprintf(logFile, "\\>\t[DEBUG]\n");
    // write current date and time
    fprintf(logFile, "%s\n", dateTime.c_str());
    // write debug info
    vfprintf(logFile, firstArg, args);
    fprintf(logFile, "\n");
    fflush(logFile);
    mutexLog.unlock();
}

/**
 * @desc writes log in W3c format to the log-file
 * @params method - c-string with request method,
 * uri - c-string with uri, status - int num means
 * response status, bytes - int num means the size
 * of response
 * @return void 
*/
void Log::writeLogW3c(const char *method, const char *uri,
                      int status, int bytes) {
    static std::ofstream logFileW3c("logW3C.txt", std::ios_base::app);
    
    if(!logFileW3c.is_open() || logLevel == Log::Level::NONE)
        return ;
    
    mutexLogW3c.lock();
    logFileW3c  << getCurrentTime() << " " << setClientIp() << " "
                << method << " " << uri << " "
                << status << " " << bytes << "\r\n";
    logFileW3c.flush();
    mutexLogW3c.unlock();
}

/**
 * @desc creates string with current date 
 * and time in format, specified by #Date
 * field in extended log file format
 * @params nope
 * @return string with date and time
*/
static std::string getCurrentDateTime() {
    std::time_t curTime = std::time(nullptr);
    std::tm* now = std::localtime(&curTime);
    std::string resCtime = ctime(&curTime);

    // day should be first
    std::string res = std::to_string(now->tm_mday);
    // then three chars of month
    res += ("-" + resCtime.substr(4, 3));
    // then a year
    res += ("-" + resCtime.substr(resCtime.length() - 5, 4));
    // and time in format hh:mm:ss
    res += (" " + getCurrentTime());

    return res;
}

/**
 * @desc creates string with current time in 
 * format hh:mm:ss 
 * @params nope
 * @return string with current time
*/
static std::string getCurrentTime() {
    time_t curTime = time(nullptr);
    std::string resCtime = ctime(&curTime);
    // cut only time from c-style time-stamp
    std::string res = resCtime.substr((resCtime.find_first_of(':') - 2), 8);
    return res;
}

/**
 * @desc writes directives #version #date 
 * and #fields into log file
 * @params none
 * @return void 
*/
void Log::startLoggingW3c() {
    std::ofstream logFileW3c("logW3C.txt", std::ios_base::app);

    if(!logFileW3c.is_open() || logLevel == Log::Level::NONE)
        return ;
    
    mutexLogW3c.lock();
    logFileW3c << "\r\n\r\n#Version: 0.9" << "\r\n" << "#Date: ";
    logFileW3c << getCurrentDateTime() << "\r\n";
    logFileW3c << "#Fields: time c-ip cs-method uri sc-status sc-bytes" << "\r\n";
    logFileW3c.flush();
    logFileW3c.close();
    mutexLogW3c.unlock();
}

void Log::closeLogFile() {
    if(!logFile)
        return ;

    fflush(logFile);
    fclose(logFile);
    logFile = nullptr;
}
