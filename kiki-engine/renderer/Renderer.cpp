#include "Renderer.hpp"

#include "utils/VulkanWindow.hpp"

#include <iostream>

namespace Kiki {
    void initialiseRenderer() {
        rutils::VulkanWindow window = rutils::makeVulkanWindow();

        std::cout << "hi :D" << std::endl;
    }
}