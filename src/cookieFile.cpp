#include <fstream>
#include <iostream>
#include <unordered_map>
#include <mutex>

#include "../include/file_utils.h"
#include "../include/cookieFile.h"

namespace cookieFile {
    std::mutex fileMutex;
}

/**
 * @desc searches for a user name in file with cookies 
 * that corresponds to passed id
 * @params searchedID - string that stores user id
 * @return user name that correspond to specified id
 * or empty line if id is not found or name absent
*/
std::string cookieFile::findNameByID(std::string searchedID) {
    
    cookieFile::fileMutex.lock();

    std::fstream cookFileStream(cookieFile::fileName, std::ios::out | std::ios::in);
    std::string line;
    std::string userId;
    std::string foundName = "";

    while (cookFileStream.is_open() && !cookFileStream.eof()) {
        getline(cookFileStream, line);
        // skip empty lines
        if(line.length() == 0)
            continue;
        // get id
        userId = line.substr(0, line.find_first_of(','));
        // compare cur id with the searched one
        if(userId.compare(searchedID) != 0)
            continue;
        // if id-s are identical - return name (2: ',' + ' ')
        foundName = line.substr(line.find_first_of(',') + 2);
        break;
    }

    cookieFile::fileMutex.unlock();

    return foundName;
}

/**
 * @desc adds new user id to the end of the file
 * (if file is absent, it will be created and id
 * will be added) 
 * @params newID - string with id of new user
 * @return void
*/
void cookieFile::addID(std::string newID) {

    cookieFile::fileMutex.lock();
    // open file in append mode (or create if not exists)
    std::ofstream cookFileStream(cookieFile::fileName, std::ios_base::app);

    // add id and separator (prepare place for future name inserting)
    cookFileStream << newID << ", " << std::endl;
    cookFileStream.close();

    cookieFile::fileMutex.unlock();
}

/**
 * @desc assigns new name to registered user 
 * @params updatedID - string with id of the
 * user, whose name would be updated, newName -
 * new name of the user
 * @return void
*/
void cookieFile::updateName(std::string updatedID, std::string newName) {
    
    cookieFile::fileMutex.lock();

    std::unordered_map<std::string, std::string> fileBuffer;

    // reading existing file to buffer
    std::ifstream cookFileStream(cookieFile::fileName);
    if(cookFileStream.is_open()) {
        readLinesToMap(fileBuffer, cookFileStream);
        cookFileStream.close();
    }

    // truncating existing file
    std::ofstream updatedCookFile;
    updatedCookFile.open(cookieFile::fileName, std::ios_base::trunc);
    
    // replacing old name associated with userId to new name
    std::unordered_map<std::string, std::string>::iterator it = fileBuffer.find(updatedID);
    if (it != fileBuffer.end())
        it->second = newName;

    // writing cookies back to the file
    it = fileBuffer.begin();
    while (it != fileBuffer.end()) {
        updatedCookFile << it->first << ", " << it->second << std::endl;
        ++it;
    }
    updatedCookFile.close();

    cookieFile::fileMutex.unlock();
}
