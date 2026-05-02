#include <ECS/System.h>
#include <Components/SimpleAnimationComponent.hpp>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

class SimpleAnimationSystem : public System {
public:
	Phase GetPhase() const override { return Phase::Update; }

	void OnUpdate(float dt) override {
		auto view = World::Get().Query<TransformComponent, SimpleAnimationComponent>();
		for (auto [entity, transform, anim] : view.each()) {
            anim.time += dt * anim.speed;

            float wave = std::sin(anim.time) * anim.distance;

            glm::vec3 localOffset = glm::vec3(0.0f);
            glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

            switch (anim.type) {
            case MsimpleAnimType::UP_DOWN:
                localOffset = glm::vec3(0.0f, wave, 0.0f);
                transform.position = anim.startPosition + (anim.startRotation * localOffset);
                break;

            case MsimpleAnimType::DOWN_UP:
                localOffset = glm::vec3(0.0f, -wave, 0.0f);
                transform.position = anim.startPosition + (anim.startRotation * localOffset);
                break;

            case MsimpleAnimType::LEFT_RIGHT:
                localOffset = glm::vec3(wave, 0.0f, 0.0f);
                transform.position = anim.startPosition + (anim.startRotation * localOffset);
                break;

            case MsimpleAnimType::RIGHT_LEFT:
                localOffset = glm::vec3(-wave, 0.0f, 0.0f);
                transform.position = anim.startPosition + (anim.startRotation * localOffset);
                break;

            case MsimpleAnimType::ROTATE_CLOCKWISE:
                localRotation = glm::angleAxis(
                    glm::radians(anim.time * anim.rotationSpeed),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                transform.rotation = anim.startRotation * localRotation;
                break;

            case MsimpleAnimType::ROTATE_COUNTERCLOCKWISE:
                localRotation = glm::angleAxis(
                    glm::radians(-anim.time * anim.rotationSpeed),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                transform.rotation = anim.startRotation * localRotation;
                break;

            default:
                break;
            }

            transform.dirty = true;
		}
	}
};
