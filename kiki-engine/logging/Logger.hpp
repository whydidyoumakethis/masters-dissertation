// Temporary logging file

#ifndef KIKI_LOGGER
#define KIKI_LOGGER

#include <iostream>

namespace klog {
    void info(std::string s);
    void error(std::string s);
}

#endif