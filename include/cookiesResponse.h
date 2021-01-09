#pragma once

#include "Response.h"

namespace Response {
    constexpr int bufferLen = 4096;
    constexpr char regFormPage[] = "startingPages/index.php";
    constexpr char greetingPage[] = "startingPages/greeting.php";
    constexpr char phpInterpreter[] = "/usr/bin/php-cgi";
    constexpr char cookFileName[] = "cookies.csv";

}

int createRootResponseBody(size_t &bodyLen,  char **body, struct s_request *req, std::string serverRoot);
