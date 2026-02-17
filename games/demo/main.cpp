#include <temp.h>
#include <glm/vec3.hpp>

#include <volk.h>

int main(int argc, char**argv) {
    temp::print("demo");
    glm::vec3 vector(0.1f, 0.2f, 3.f);

    std::cout << vector.r << " " << vector.g << " " << vector.b << std::endl;

    VkResult volkInitilialize();

    return 0;
}