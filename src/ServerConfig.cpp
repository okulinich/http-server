#include <cstring>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <array>

#include "../include/ServerConfig.h"
#include "../include/Log.h"
#include "../include/file_utils.h"

extern "C" {
    #include "../include/sys_utils.h"
}

ServerConfig::ServerConfig(std::string fileName) {
    confFileName = fileName;
    confFileName = "conf/config1.csv";
    port = "3819";
    defaultFile = "index.html";
    rootFolder = "resources/";
    ipv = 0;
    interface = "0";
    backlog = 10;
    curLvl = Log::Level::ERROR;
    timeout = 5;
    maxReq = 100;
}

/**
 * @desc reads configuration lines from file
 * and applies them if they are correct
 * @params none 
 * @return void
*/
void ServerConfig::readFromFile() {
    std::ifstream confStream;

    confStream.open(confFileName);
    //error openning file
    if(!confStream.is_open()) {
        Log::writeLog(Log::Level::ERROR, "Config-file: %s\n", strerror(errno));
        Log::writeLog(Log::Level::WARNING, "Default configurations were applied!");
        return ;
    }

    std::unordered_map<std::string, std::string> confLines;
    std::unordered_map<std::string, std::string>::iterator configName;

    readLinesToMap(confLines, confStream);
    
    // configuration parameters
    std::array<std::string, 9> params = { "port", "root", "doc", "ipv", "interface", "backlog", "log", "timeout", "maxReq" };
    // array of lambdas to validate and update each parameter
    void (*updFunc[9])(ServerConfig *, std::string) = {
        [](ServerConfig *servConf, std::string value) { 
            int portNum = atoi(value.c_str());
            if (portNum == 80 || (portNum > 1023 && portNum < 65353))
                servConf->port = value;
        },
        [](ServerConfig *servConf, std::string value) { 
            if(checkForWildcards(value.c_str()) && value[0] != '/')
                servConf->rootFolder = value;
        },
        [](ServerConfig *servConf, std::string value) { 
            if(checkForWildcards(value.c_str()) && value[0] != '/')
                servConf->defaultFile = value;
        },
        [](ServerConfig *servConf, std::string value) { 
            if(value[0] == '4' || value[0] == '6')
                servConf->ipv = atoi(value.c_str());
        },
        [](ServerConfig *servConf, std::string value) { 
            if (value.compare("none") != 0 && value[0] != '0')
                servConf->interface = value;
        },
        [](ServerConfig *servConf, std::string value) { 
            int blog = atoi(value.c_str());
            if(blog > 0)
                servConf->backlog = blog;
        },
        [](ServerConfig *servConf, std::string value) { 
            if(value.compare("NONE") == 0)
                servConf->curLvl = Log::Level::NONE;
            else if(value.compare("DEBUG") == 0)
                servConf->curLvl = Log::Level::DEBUG;
            else if(value.compare("WARNING") == 0)
                servConf->curLvl = Log::Level::WARNING;
        },
        [](ServerConfig *servConf, std::string value) { 
            int time = atoi(value.c_str());
            if(time > 0)
                servConf->timeout = time;
        },
        [](ServerConfig *servConf, std::string value) { 
            int resp = atoi(value.c_str());
            if(resp > 0)
                servConf->maxReq = resp;
        }
    };

    for(unsigned i = 0; i < params.size(); i++) {
        // find parameter name in the map of lines from file
        configName = confLines.find(params[i]);
        // if parameter name found and its value exists -> validate & update
        if(configName != confLines.end() && configName->second.length() > 0)
            updFunc[i](this, configName->second);
    }
}
