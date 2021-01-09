#pragma once

#include <string>
#include "Log.h"

struct ServerConfig {
    public:
        std::string confFileName,
                    port,
                    defaultFile,
                    rootFolder,
                    interface;
        bool logging;
        Log::Level curLvl;
        int backlog,
            ipv,
            timeout,
            maxReq;

        ServerConfig(std::string confFileName);
        void readFromFile();
};
