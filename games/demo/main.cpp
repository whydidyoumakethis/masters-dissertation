#include <kiki.h>
#include <glm/vec3.hpp>

#include <volk.h>
#include <entt/entt.hpp>
#include <GLFW/glfw3.h>

//#include <GltfLoader/GltfLoader.h>
#include <GltfLoader/GltfLoaderAssimp.h>
#include "renderer/SceneManager.hpp"

#include <Jolt/Jolt.h>

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




// // TODO: temporary glfw input handling
// void glfwCallback(GLFWwindow* aWindow, int aKey, int /*aScanCode*/, int aAction, int /*aModifierFlags*/) {
//     if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction) {
// 		glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
// 	}
// }


int main(int argc, char** argv) {
    //using namespace entt::literals; // to simplify the use of hashed_string 
    //temp::print("demo");
    //glm::vec3 vector(0.1f, 0.2f, 3.f);

    //spdlog::info("{0} {1} {2}", vector.r, vector.g , vector.b);
    //// std::cout << vector.r << " " << vector.g << " " << vector.b << std::endl;

    //// VkResult volkInitilialize();

    //// glfwSetCharCallback();

    //if (!glfwInit())
    //    return -1;
    //entt::registry registry;

    //for (auto i = 0u; i < 10u; ++i) {
    //    const auto entity = registry.create();
    //    registry.emplace<position>(entity, i * 1.f, i * 1.f);
    //    if (i % 2 == 0) { registry.emplace<velocity>(entity, i * .1f, i * .1f); }
    //}
    //update(registry);

    //

    //const auto player = registry.create();
    //registry.emplace<tag>(player, "player"_hs); // Simplified version of hashed_string
    //registry.emplace<position>(player, 2.f, 4.f);
    //registry.emplace<velocity>(player, 3.f, 4.f);
    //const auto enemy = registry.create();
    //registry.emplace<tag>(enemy, "enemy"_hs);
    //registry.emplace<position>(enemy, 9.f, 9.f);
    //registry.emplace<velocity>(enemy, 9.f, 9.f);
    //auto view = registry.view<const position,velocity,tag>();
    //for (auto [entity, pos, vel, tag] : view.each()) {
    //    if (tag.string == "player"_hs) {
    //        spdlog::info("player's position is {0} {1}",pos.x,pos.y);
    //        spdlog::info("player's velocity is {0} {1}", vel.dx, vel.dy);
    //    }
    //}
    //add_context(registry);
    //get_context(registry);

    //// Temporary game loop
    //Kiki::RenderManager& renderManager = Kiki::RenderManager::get();
    //Kiki::WindowInfo info;
    //info.fullscreen = false;
    //info.monitor = 0;
    //// info.width = 0;
    //// info.height = 0;
    //// info.decorations = false;
    //info.resizeable = false;

    //renderManager.initialise(info);

    //GLFWwindow* window = renderManager.getWindow();

    //// create temporary glfw callback
    //glfwSetKeyCallback(window, &glfwCallback);

    //while (!glfwWindowShouldClose(window)) {
    //    renderManager.nextFrame();
    //}

    //renderManager.shutdown();

    //return 0;

	Kiki::Engine engine;
	engine.Init();

    // -------------------Simple test of some functions of the physical module-----------------
    // auto* physics = engine.GetSystem<Kiki::PhysicsSystem>();
    // // Create a static ground surface
    // auto floor = GameObject::Create("StaticFloor");
    // floor->SetPosition({ 0.0f, 0.0f, 0.0f });
    // floor->AddComponent<Kiki::BoxColliderComponent>(glm::vec3(50.0f, 1.0f, 50.0f));
    // floor->AddComponent<Kiki::RigidBodyComponent>(JPH::EMotionType::Static, (uint16_t)0);

	// // Create a dynamic ball
    // auto ball = GameObject::Create("FallingBall");
    // ball->SetPosition({ 0.0f, 10.0f, 0.0f }); //10m high
    // ball->AddComponent<Kiki::SphereColliderComponent>(0.5f);
    // ball->AddComponent<Kiki::RigidBodyComponent>(JPH::EMotionType::Dynamic, (uint16_t)1, 0.5f);
    // physics->AddImpulse(ball->GetEntity(), glm::vec3(100.0f, 0.0f, 0.0f));

    // spdlog::info("Physics World Initialized. Watch the ball fall!");

    auto& registry = World::Get().Registry();
    auto road = World::Get().CreateEntity();

    std::vector<glm::vec3> p = {
        glm::vec3(-1.f, 0.f, -6.f), // v0
        glm::vec3(-1.f, 0.f, +6.f), // v1
        glm::vec3(+1.f, 0.f, +6.f), // v2
        glm::vec3(+1.f, 0.f, -6.f) // v3
    };

    std::vector<std::uint32_t> i = { 0, 1, 2, 0, 2, 3 };

    std::vector<glm::vec2> c = {
        glm::vec2(0.f, -6.f), // t0
        glm::vec2(0.f, +6.f), // t1
        glm::vec2(1.f, +6.f), // t2
        glm::vec2(1.f, -6.f) // t3
    };
    //
    stbi_set_flip_vertically_on_load( 1 );

    // Load base image
    int baseWidthi, baseHeighti, baseChannelsi;

    stbi_uc* data = stbi_load( (std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/asphalt.png").c_str(), &baseWidthi, &baseHeighti, &baseChannelsi, 4 /* want 4 c h a n n e l s = RGBA */);
    //
    registry.emplace<TransformComponent>(road);
    registry.emplace<MeshComponent>(road, Kiki::SceneManager::get().createMesh(p, i, c));
    registry.emplace<MaterialComponent>(road, Kiki::SceneManager::get().createMaterial(data, baseWidthi, baseHeighti));

    // Vertex data
    std::vector<glm::vec3> points = {
        glm::vec3(-1.5f, +1.5f, -4.f), // v0
        glm::vec3(-1.5f, -0.5f, -4.f), // v1
        glm::vec3(+1.5f, -0.5f, -4.f), // v2
        glm::vec3(+1.5f, +1.5f, -4.f) // v3
    };

    std::vector<std::uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

    std::vector<glm::vec2> tex = {
        glm::vec2(0.f, 1.f), // t0
        glm::vec2(0.f, 0.f), // t1
        glm::vec2(1.f, 0.f), // t2
        glm::vec2(1.f, 1.f) // t3
    };

    auto explosion = World::Get().CreateEntity();

    registry.emplace<TransformComponent>(explosion);
    registry.emplace<MeshComponent>(explosion, Kiki::SceneManager::get().createMesh(points, indices, tex));

    data = stbi_load( (std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/explosion.png").c_str(), &baseWidthi, &baseHeighti, &baseChannelsi, 4 /* want 4 c h a n n e l s = RGBA */);

    std::cout << baseChannelsi << std::endl;

    registry.emplace<MaterialComponent>(explosion, Kiki::SceneManager::get().createMaterial(data, baseWidthi, baseHeighti));
    registry.emplace<TransparencyComponent>(explosion, 0.5f, true);

    auto test_cube = World::Get().CreateEntity();
    registry.emplace<TransformComponent>(test_cube);

    Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / "test_cube_tex.glb");
    Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "test_cube_tex.glb");

    registry.emplace<MeshComponent>(test_cube, Kiki::SceneManager::get().createMesh(mesh.vertices, mesh.indices, mesh.uvs));
    const bool isCompressed = !texture.rawData.empty();
    unsigned char* texPtr = isCompressed
        ? (unsigned char*)texture.rawData.data()
        : (unsigned char*)texture.data.data();
    int texSize = isCompressed
        ? static_cast<int>(texture.rawData.size())
        : static_cast<int>(texture.data.size() * sizeof(RGBA));

    // registry.emplace<MaterialComponent>(test_cube,
    //     Kiki::SceneManager::get().createMaterial(texture.rawDataPtr, texture.width, texture.height));

    Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
    Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

    engine.Run();

}
