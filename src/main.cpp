#include <iostream>

#include "../include/Server.h"
#include "../include/ServerConfig.h"
#include "../include/Log.h"

extern "C" {
    #include "../include/sys_utils.h"
}

/**
  * @desc It's a simple echo server, that processes connections
  * one by one, but multiple connections support will be added
  * soon. Server could be configured using conf/config1 file,
  * or you can create your own config file somewhere in server
  * working directory and pass relative path to this fill to
  * ServerConfig class costructor
*/
int main() {
    handleSignalInt();
    
    ServerConfig newConfig("conf/config1.csv");
    newConfig.readFromFile();
    
    Server server(newConfig);
    Log::logLevel = newConfig.curLvl;

    if(!server.createAndBind())
        exit(EXIT_FAILURE);

    if(!server.run())
        exit(EXIT_FAILURE);

    return 0;
}
