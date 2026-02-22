#include <temp.h>
#include <spdlog/spdlog.h>
void temp::print(std::string s) {
    //std::cout << s << std::endl;
    spdlog::info(s);
}