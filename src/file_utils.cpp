#include <cstring>

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "../include/Response.h"
#include "../include/parser.h"

/**
 * @desc reads one chunk from the file
 * starting with specified position
 * @params resourse - stream, associated 
 * with a file, position - starting pos
 * for reading, content pointer to c-string,
 * where DINAMIC memory will be allocated 
 * to store body of request, len - pointer
 * to int, where total length of body stores
 * @return number of bytes readed from 
 * file
*/
size_t readSingleChunk(std::ifstream &resourse, unsigned long &position, char **content, size_t *len) {
    char buf[Response::maxChunkSize] = { 0 };
    int bytesRead = 0;
    std::stringstream sizeHex;
    int lenHex = 0;

    resourse.read(buf, Response::maxChunkSize);
    // remeber position where reading stopped
    position = resourse.tellg();
    // create chunk entity: LEN in hex \r\n chunk
    bytesRead = resourse.gcount();
    sizeHex << std::hex << bytesRead << "\r\n";
    lenHex = sizeHex.str().length();

    *len = lenHex + bytesRead;
    *content = new char[*len];
    memcpy(*content, sizeHex.str().c_str(), lenHex);
    memcpy(&(*content)[lenHex], buf, bytesRead);

    return bytesRead;
}

/**
 * @desc opens requested file as binary file
 * and reads it's content into specified by 
 * passed pointer c-string, that is ALLOCATED
 * DINAMICLY in case of successfull file opening.
 * In case of errors NO dinamic memmory allocated.
 * @params pathToFile - relative path to file
 * (server root + uri), content - pointer to 
 * c-string where the result would be placed,
 * len - will store length of content
 * @return 0 in case of success, http status
 * code otherwise
*/
int getFileContent(std::string pathToFile, char **content, size_t *len, unsigned long &position) {
    std::ifstream resourse;

    resourse.open(pathToFile, std::ios::in | std::ios::binary | std::ios::ate);
    if(!resourse.is_open()) {
        if(errno == 2)
            return ERROR_NOT_FOUND;
        if(errno == 13)
            return ERROR_NO_PERMISSION;
        return ERROR_INTERNAL_SERVER;
    }
    // if reading of large file was not over
    if(position > 0) {
        // go to position where reading stopped
        resourse.seekg(position);
        // read more one chunk
        int chunkSize = readSingleChunk(resourse, position, content, len);
        // if readed less than chunk size = eof reached
        if(chunkSize < Response::maxChunkSize) {
            position = 0;
            return SUCCESS;
        }
        return CHUNKED;
    }
    // get file size and go back to the beginning
    std::streampos size = resourse.tellg();
    resourse.seekg(0, std::ios::beg);
    // if file size is less than chunk size -> read the whole file
    if(size < Response::maxChunkSize) {
        *content = new char[size];
        resourse.read(*content, size);
        resourse.close();
        *len = size;
        return SINGLE_RESPONSE;
    }
    // read one chunk and save position where reading stopped
    else {
        readSingleChunk(resourse, position, content, len);
        return CHUNKED;
    }
}

/**
 * @desc simply checks wether the URI is valid or not
 * and extracts file name
 * @params param - - struct with additional server 
 * params (Log object, timeout, etc.), req - - struct filled
 * by request parser, fileName - var passed by reference, where
 * extracted file name should be placed, defaultFile - 
 * c-string that stores name of the default file that would
 *  be returned when root uri ('/') requested
 * @return HTTP error code in case of failure, 0 otherwise
*/
int filenameFromUri(std::string uri, std::string &fileName, std::string defaultFile) {
    if(uri.compare("*") == 0)
        return ERROR_NOT_IMPLEMENTED;
    //if uri is absolute -> take part after last domain
    if(uri.length() > 0 && uri.substr(0, 7).compare("http://") == 0) {
        //locate first "/" after all domain names
        int pos = uri.find_first_of('/', 8);
        //if path in uri is not just root folder
        if(uri.length() - pos > 1)
            fileName = uri.substr(pos + 1); //skip first "/"
        else
            fileName = defaultFile;
    }
    //if uri is abs path -> simply copy it
    else {
        if(uri.compare("/") == 0)
            fileName = defaultFile;
        else
            fileName = uri.substr(1); //skip first "/"
    }
    return 0;
}

/**
 * @desc reads configuration lines from the
 * filestream and adds those lines to an 
 * unordered map as "key: value"
 * @params confLines - unordered map of
 * strings, conf - input file stream
 * associated with configuration file 
 * @return void
*/
void readLinesToMap(std::unordered_map<std::string, std::string> &confLines,
                                                                std::ifstream &conf) {
    std::string buffer;
    size_t delimPos = 0;

    // reading whole configuration file
    for(int i = 0; !conf.eof(); i++) {
        // adding every configuration line to map
        getline(conf, buffer);
        delimPos = buffer.find_first_of(',');
        // if ':' symb is absent -> skip string
        if(delimPos == std::string::npos)
            continue;
        confLines.insert(
                        std::make_pair<std::string, std::string>
                        (buffer.substr(0, delimPos), buffer.substr(delimPos + 2))
                        );
    }
}
