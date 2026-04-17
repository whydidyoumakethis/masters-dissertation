#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <volk.h>
#include <glm/gtx/matrix_decompose.hpp> 

#include "physics/PhysicsComponents.hpp" 
#include "physics/PhysicsUtils.hpp"   
#include "physics/PhysicsSystem.hpp" 
#include "Components/MiscComponent.hpp"

#include <spdlog/spdlog.h>

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

     int SceneManager::createMaterial(Mtexture& texture) {
         materials.emplace_back(RenderManager::get().allocateMaterial(texture));

         return materials.size() - 1;
     }

    Material const& SceneManager::getMaterial(int id) {
        return materials[id];
    }

    int SceneManager::createMesh(std::vector<glm::vec3> const& positions, std::vector<std::uint32_t> const& indices, std::vector<glm::vec3> const& normals, std::vector<glm::vec2> const& texCoords) {
        std::vector<float> p, t, n;

        for (glm::vec3 pos : positions) {
            p.emplace_back(pos.x);
            p.emplace_back(pos.y);
            p.emplace_back(pos.z);
        }

        for (glm::vec2 texCoord : texCoords) {
            t.emplace_back(texCoord.x);
            t.emplace_back(texCoord.y);
        }

        for (glm::vec3 norm : normals) {
            n.emplace_back(norm.x);
            n.emplace_back(norm.y);
            n.emplace_back(norm.z);
        }

        meshes.emplace_back(RenderManager::get().allocateMesh(p, indices, n, t));

        return meshes.size() - 1;
    }

    Mesh& SceneManager::getMesh(int id) {
        return meshes[id];
    }

    void SceneManager::clearLevel() {
        vkDeviceWaitIdle(RenderManager::get().getDevice());

        auto& registry = World::Get().Registry();

        auto view = World::Get().Query<MeshComponent>();

        for (auto [e, meshComponent] : view.each()) {
            registry.destroy(e);
        }

        materials.clear();
        meshes.clear();
    }

    entt::entity SceneManager::loadModel(const std::string path, const std::string name, PhysicsType type) {
        auto model = World::Get().CreateEntity();
        auto& registry = World::Get().Registry();

        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / path, 0);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / path, 0);
        registry.emplace<TransformComponent>(model);
        registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs));
        registry.emplace<TagComponent>(model, entt::hashed_string(name.c_str()), name);
        JPH::Ref<JPH::Shape> colliderShape;
        JPH::EMotionType joltMotionType;
        uint16_t joltLayer;

        switch (type) {
        case PhysicsType::Static:
            colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
            joltMotionType = JPH::EMotionType::Static;
            joltLayer = 0; // NON_MOVING
            break;

        case PhysicsType::Dynamic:
            colliderShape = CreateConvexHull(mesh.vertices);
            joltMotionType = JPH::EMotionType::Dynamic;
            joltLayer = 1; // MOVING
            break;

        case PhysicsType::Kinematic:
            colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
            joltMotionType = JPH::EMotionType::Kinematic;
            joltLayer = 0;
            break;
        }

        if (colliderShape) {
            registry.emplace<MeshColliderComponent>(model, colliderShape);
            registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer);
			registry.emplace<PhysicalAttributesComponent>(model);
            spdlog::info("Model {} loaded as {}", path,
                type == PhysicsType::Static ? "Static" : (type == PhysicsType::Dynamic ? "Dynamic" : "Kinematic"));
        }

        if (texture.hastexture) {

     
            registry.emplace<MaterialComponent>(model,
                    createMaterial(texture));
        }

        registry.emplace<ColourComponent>(model, glm::vec3(1, 0, 0));

        Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
        Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
		return model;
    }


    void SceneManager::loadScene(const Mscene& scene) {
        // TEMP SOLUTION

        for (int i = 0; i < scene.instances.size(); i++) {
           auto model = World::Get().CreateEntity();
            auto& registry = World::Get().Registry();
			const auto& instance = scene.instances[i];
            const Mmesh& mesh = scene.meshes[instance.meshIndex];
            const Mtexture& texture = scene.textures[mesh.matIndex];
            auto& transform = registry.emplace<TransformComponent>(model);
            glm::vec3 skew;
            glm::vec4 perspective;
			glm::decompose(scene.instances[i].transform, transform.scale, transform.rotation, transform.position, skew, perspective);
            transform.rotation = glm::conjugate(transform.rotation);
            //transform.scale = {1, 1, 1}; // TODO: this is a temp fix, will probably cause issues

            registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs));
            if (texture.hastexture) {
                materials.emplace_back(RenderManager::get().allocateMaterial(texture));
				int id = materials.size() - 1;
                registry.emplace<MaterialComponent>(model, id);
				if (texture.mode == alphaMode::MASK) {
                    registry.emplace<TransparencyComponent>(model); // yeah idk what else to do other then just have this added
                }
            }
            registry.emplace<ColourComponent>(model, glm::vec3(0.3f, 0.3f, 0.3f));
            //Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
			//Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

			// Physics setup

            registry.emplace<TagComponent>(model, entt::hashed_string(mesh.name.c_str()), mesh.name);
            JPH::Ref<JPH::Shape> colliderShape;
            JPH::EMotionType joltMotionType;
            uint16_t joltLayer;
            //COPIED WHAT WAS DONE IN LOAD MODEL, PLEASE WORK ON THIS FUNCTION FROM NOW AS PREVIOUS FUNCTION HAS DEPRECATED FUNCTIONS
			switch (instance.bodyType) {
			case MbodyType::STATIC:
                colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                joltMotionType = JPH::EMotionType::Static;
                joltLayer = 0; // NON_MOVING
                break;
            case MbodyType::DYNAMIC:
                colliderShape = CreateConvexHull(mesh.vertices);
                joltMotionType = JPH::EMotionType::Dynamic;
                joltLayer = 1; // MOVING
				break;
            case MbodyType::KINEMATIC:
                colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                joltMotionType = JPH::EMotionType::Kinematic;
				joltLayer = 0;
                break;
             }
            if (colliderShape) {
                registry.emplace<MeshColliderComponent>(model, colliderShape);
				registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer);
                registry.emplace<PhysicalAttributesComponent>(model);
            }

			// Misc tags
            if (instance.miscTag != MmiscTags::NONE) {
                registry.emplace<MiscComponent>(model, instance.miscTag);
			}
        }
    }

    bool SceneManager::validMaterial(int id) {
        return id >= 0 && id < materials.size();
    }

    bool SceneManager::validMesh(int id) {
        return id >= 0 && id < meshes.size();
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}