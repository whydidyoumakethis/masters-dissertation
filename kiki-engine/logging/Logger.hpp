// Temporary logging file

#ifndef KIKI_LOGGER
#define KIKI_LOGGER

#include <iostream>

namespace log {
    void info(std::string s);
    void error(std::string s);
}

#endif