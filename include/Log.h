#pragma once

#include <fstream>

/**
 * @desc to write a log in w3c format
 * you need to make 3 calls: startLoggingW3c()
 * - this call writes to the file some starting
 * info (directives), setClientIp() - saves 
 * address of current client (requeired to write
 * a log), and finally writeLogW3c() that writes
 * well formated log into the file.
 * To write a debug log you only need to call
 * writeLog() method and pass parameters as you
 * pass them to prinf(). that is static method,
 * so you don't even need to create a class object.
*/

namespace Log {

    enum class Level { 
        NONE, 
        DEBUG,
        WARNING,
        ERROR
    };

    extern Level logLevel;
    
    const char *setClientIp(const char *clIP);
    void writeLog(Log::Level lvl, const char *firstArg, ...);
    void startLoggingW3c();
    void writeLogW3c(const char *method, const char *uri,
                                    int status, int bytes);
    void closeLogFile();
}
