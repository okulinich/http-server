#pragma once

#include <iostream>

namespace cookieFile {
    constexpr char fileName[] = "cookies.csv";

    void updateName(std::string id, std::string name);
    void addID(std::string newID);
    std::string findNameByID(std::string id);
}
