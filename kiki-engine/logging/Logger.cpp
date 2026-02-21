#include "Logger.hpp"

// Temporary logging file
namespace klog {
    void error(std::string s) {
        std::cout << s << std::endl;
    }

    void log(std::string s) {
        std::cout << s << std::endl;
    }
}