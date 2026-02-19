#include <temp.h>
#include <glm/vec3.hpp>

#include <volk.h>
#include <entt/entt.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
struct position {
    float x;
    float y;
};

struct velocity {
    float dx;
    float dy;
};
void update(entt::registry& registry) {
    auto view = registry.view<const position, velocity>();

    // use a callback
    view.each([](const auto& pos, auto& vel) { /* ... */
        std::cout << pos.x <<"   " << pos.y << std::endl;
        
        });

    // use an extended callback
    view.each([](const auto entity, const auto& pos, auto& vel) { /* ... */ });

    // use a range-for
    for (auto [entity, pos, vel] : view.each()) {
        // ...
    }

    //// use forward iterators and get only the components of interest
    //for (auto entity : view) {
    //    auto& vel = view.get<velocity>(entity);
    //    // ...
    //}
}

int main(int argc, char**argv) {
    temp::print("demo");
    glm::vec3 vector(0.1f, 0.2f, 3.f);

    std::cout << vector.r << " " << vector.g << " " << vector.b << std::endl;

    // VkResult volkInitilialize();

    // glfwSetCharCallback();

    if (!glfwInit())
        return -1;
    entt::registry registry;

    for (auto i = 0u; i < 10u; ++i) {
        const auto entity = registry.create();
        registry.emplace<position>(entity, i * 1.f, i * 1.f);
        if (i % 2 == 0) { registry.emplace<velocity>(entity, i * .1f, i * .1f); }
    }
    update(registry);
    return 0;
}