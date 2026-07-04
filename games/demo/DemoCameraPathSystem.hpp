#pragma once

#include <kiki.h>

#include <algorithm>
#include <vector>

struct DemoCameraPathPoint {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 lookAt = { 0.0f, 0.0f, 0.0f };
    float durationToNext = 3.0f;
};

class DemoCameraPathSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnStart() override {
        path = {
            { glm::vec3(0.0f, 3.0f, 8.0f), glm::vec3(0.0f, 1.5f, 0.0f), 3.0f },
            { glm::vec3(6.0f, 4.0f, 2.0f), glm::vec3(0.0f, 1.5f, 0.0f), 3.0f },
            { glm::vec3(0.0f, 5.0f, -8.0f), glm::vec3(0.0f, 1.5f, 0.0f), 3.0f },
        };

        MakeOnlyMainCamera();
        SnapToPoint(0);
    }

    void OnUpdate(float dt) override {
        if (inputManager.isKeyJustDown(GLFW_KEY_P)) {
            StartPath();
        }

        if (inputManager.isKeyJustDown(GLFW_KEY_O)) {
            StopPath();
            SnapToPoint(0);
        }

        if (!playing || path.size() < 2) return;

        UpdatePath(dt);
    }

private:
    Kiki::InputManager& inputManager = Kiki::InputManager::get();
    Kiki::Camera camera;
    std::vector<DemoCameraPathPoint> path;
    std::size_t segmentIndex = 0;
    float segmentTime = 0.0f;
    bool playing = false;

    void MakeOnlyMainCamera() {
        auto& registry = World::Get().Registry();
        auto cameras = World::Get().Query<CameraComponent>();
        for (auto [entity, cam] : cameras.each()) {
            cam.isMain = false;
        }

        registry.get<CameraComponent>(camera.camera).isMain = true;
    }

    void StartPath() {
        segmentIndex = 0;
        segmentTime = 0.0f;
        playing = path.size() >= 2;
        SnapToPoint(0);
        spdlog::info("[DemoCameraPath] Started camera path");
    }

    void StopPath() {
        playing = false;
        segmentIndex = 0;
        segmentTime = 0.0f;
    }

    void UpdatePath(float dt) {
        const DemoCameraPathPoint& from = path[segmentIndex];
        const DemoCameraPathPoint& to = path[segmentIndex + 1];
        const float duration = std::max(0.001f, from.durationToNext);

        segmentTime += dt;

        float t = glm::clamp(segmentTime / duration, 0.0f, 1.0f);
        t = t * t * (3.0f - 2.0f * t);

        SetCameraTransform(
            glm::mix(from.position, to.position, t),
            glm::mix(from.lookAt, to.lookAt, t)
        );

        if (segmentTime >= duration) {
            segmentIndex++;
            segmentTime = 0.0f;

            if (segmentIndex >= path.size() - 1) {
                playing = false;
                SnapToPoint(path.size() - 1);
                spdlog::info("[DemoCameraPath] Finished camera path");
            }
        }
    }

    void SnapToPoint(std::size_t index) {
        if (path.empty()) return;
        index = std::min(index, path.size() - 1);
        SetCameraTransform(path[index].position, path[index].lookAt);
    }

    void SetCameraTransform(const glm::vec3& position, const glm::vec3& lookAt) {
        auto& transform = World::Get().Registry().get<TransformComponent>(camera.camera);
        transform.position = position;

        glm::vec3 forward = lookAt - position;
        if (glm::dot(forward, forward) < 0.0001f) {
            forward = glm::vec3(0.0f, 0.0f, -1.0f);
        }

        transform.rotation = glm::quatLookAt(glm::normalize(forward), glm::vec3(0.0f, 1.0f, 0.0f));
        transform.dirty = true;
    }
};
