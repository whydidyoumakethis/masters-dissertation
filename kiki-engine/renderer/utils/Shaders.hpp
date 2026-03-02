#ifndef KIKI_RENDERER_SHADERS
#define KIKI_RENDERER_SHADERS

#include <vector>
#include <cstdint>

namespace rutils {
    std::vector<std::uint32_t> loadShader(char const* aPath);
}

#endif