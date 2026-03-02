#include <kiki.h>
#include <glm/vec3.hpp>

#include <volk.h>
#include <entt/entt.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "renderer/WindowInfo.hpp"
#include "renderer/RenderManager.hpp"
#include "input/InputManager.hpp"

#include <spdlog/spdlog.h>

struct position {
    float x;
    float y;
};
struct velocity {
    float dx;
    float dy;
};
struct tag {
    entt::hashed_string string;
};
void update(entt::registry& registry) {
    auto view = registry.view<const position, velocity>();

    // use a callback
    view.each([](const auto& pos, auto& vel) { /* ... */
        spdlog::info("Positions are {0} {1}", pos.x, pos.y);
        
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
void add_context(entt::registry& registry) {
    // one type for one value in default
    registry.ctx().emplace<int>(42);
}
void get_context(entt::registry& registry) {
    // one type for one value in default
    int int_value = registry.ctx().get<int>();
    spdlog::info("score: {0}", int_value);
}




// TODO: temporary glfw input handling
void glfwCallback(GLFWwindow* aWindow, int aKey, int /*aScanCode*/, int aAction, int /*aModifierFlags*/) {
    if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction) {
		glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
	}
}


int main(int argc, char** argv) {
    using namespace entt::literals; // to simplify the use of hashed_string 
    temp::print("demo");
    glm::vec3 vector(0.1f, 0.2f, 3.f);

    spdlog::info("{0} {1} {2}", vector.r, vector.g , vector.b);
    // std::cout << vector.r << " " << vector.g << " " << vector.b << std::endl;

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

    

    const auto player = registry.create();
    registry.emplace<tag>(player, "player"_hs); // Simplified version of hashed_string
    registry.emplace<position>(player, 2.f, 4.f);
    registry.emplace<velocity>(player, 3.f, 4.f);
    const auto enemy = registry.create();
    registry.emplace<tag>(enemy, "enemy"_hs);
    registry.emplace<position>(enemy, 9.f, 9.f);
    registry.emplace<velocity>(enemy, 9.f, 9.f);
    auto view = registry.view<const position,velocity,tag>();
    for (auto [entity, pos, vel, tag] : view.each()) {
        if (tag.string == "player"_hs) {
            spdlog::info("player's position is {0} {1}",pos.x,pos.y);
            spdlog::info("player's velocity is {0} {1}", vel.dx, vel.dy);
        }
    }
    add_context(registry);
    get_context(registry);

    // Temporary game loop
    Kiki::RenderManager& renderManager = Kiki::RenderManager::get();
    Kiki::WindowInfo info;
    info.fullscreen = false;
    info.monitor = 0;
    // info.width = 0;
    // info.height = 0;
    // info.decorations = false;
    info.resizeable = false;

    renderManager.initialise(info);

    GLFWwindow* window = renderManager.getWindow();

    // create temporary glfw callback
    glfwSetKeyCallback(window, &glfwCallback);

    while (!glfwWindowShouldClose(window)) {
        renderManager.nextFrame();
    }

    renderManager.shutdown();

    return 0;
}
