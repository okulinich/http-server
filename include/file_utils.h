#pragma once

#include <fstream>
#include <unordered_map>

#include "Response.h"

size_t readSingleChunk(std::ifstream &resourse, unsigned long &position, char **content, size_t *len);
int getFileContent(std::string pathToFile, char **content, size_t *len, unsigned long &position);
int filenameFromUri(std::string uri, std::string &fileName, std::string defaultFile);
void readLinesToMap(std::unordered_map<std::string, std::string> &confLines, std::ifstream &conf);
