#pragma once

#include <kiki.h>
#include <Components/CamPathPointComponent.hpp>
#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <vector>

struct DemoCameraPathPoint {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float durationToNext = 3.0f;
    int pathIndex = -1;
};

class DemoCameraPathSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnStart() override {
        RebuildPathFromScene();
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

    void RebuildPathFromScene() {
        path.clear();

        auto view = World::Get().Query<TransformComponent, CameraPathPointComponent>();

        for (auto [entity, transform, point] : view.each()) {
            DemoCameraPathPoint pathPoint;
            pathPoint.position = transform.position;
            pathPoint.rotation = transform.rotation;
            pathPoint.durationToNext = point.durationToNext;
            pathPoint.pathIndex = point.pathIndex;
            path.push_back(pathPoint);
        }

        std::sort(path.begin(), path.end(),
            [](const DemoCameraPathPoint& a, const DemoCameraPathPoint& b) {
                if (a.pathIndex < 0) return false;
                if (b.pathIndex < 0) return true;
                return a.pathIndex < b.pathIndex;
            });

        if (path.empty()) {
            spdlog::warn("[DemoCameraPath] No camera path points found in the loaded scene");
            return;
        }

        for (const auto& point : path) {
            if (point.pathIndex < 0) {
                spdlog::warn("[DemoCameraPath] Found a camera path point with no valid path index");
                break;
            }
        }

        spdlog::info("[DemoCameraPath] Loaded {} camera path point(s)", path.size());
    }

    void MakeOnlyMainCamera() {
        auto& registry = World::Get().Registry();
        auto cameras = World::Get().Query<CameraComponent>();
        for (auto [entity, cam] : cameras.each()) {
            cam.isMain = false;
        }

        registry.get<CameraComponent>(camera.camera).isMain = true;
    }

    void StartPath() {
        RebuildPathFromScene();
        segmentIndex = 0;
        segmentTime = 0.0f;
        playing = path.size() >= 2;
        SnapToPoint(0);

        if (playing) {
            spdlog::info("[DemoCameraPath] Started camera path");
        } else {
            spdlog::warn("[DemoCameraPath] Need at least 2 camera path points to start");
        }
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
            glm::slerp(from.rotation, to.rotation, t)
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
        SetCameraTransform(path[index].position, path[index].rotation);
    }

    void SetCameraTransform(const glm::vec3& position, const glm::quat& rotation) {
        auto& transform = World::Get().Registry().get<TransformComponent>(camera.camera);
        transform.position = position;
        transform.rotation = glm::normalize(rotation);
        transform.dirty = true;
    }
};
