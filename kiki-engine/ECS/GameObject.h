#pragma once
#include "World.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// defult components for game objects

struct TransformComponent {
    glm::vec3 position = { 0, 0, 0 };
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = { 1, 1, 1 };
    Entity    parent = NullEntity;
    glm::mat4 worldMatrix = glm::mat4(1.0f);
	bool      dirty = true; // flag to indicate if the transform has changed and needs to be updated
};

struct ActiveComponent {
    bool active = true;
};

struct TagComponent {
    entt::hashed_string tag; // Example usage: TagComponent.tag == "player"_hs
	std::string name; // for debugging purposes
};

class GameObject {
public:
    static std::shared_ptr<GameObject> Create(const std::string& name = "GameObject") {
        return std::make_shared<GameObject>(name);
    }
    explicit GameObject(const std::string& name) {
		auto& reg = World::Get().Registry();
        _entity = World::Get().CreateEntity();
		// default components
		reg.emplace<TransformComponent>(_entity);
		reg.emplace<ActiveComponent>(_entity);
		reg.emplace<TagComponent>(_entity, entt::hashed_string(name.c_str()),name);
    }
    ~GameObject() {
        World::Get().DestroyEntity(_entity);
    }

	Entity GetEntity() const { return _entity; }

    // ©¤©¤ Quick access to basic properties ©¤©¤
    const std::string& GetName() const {
        auto& reg = World::Get().Registry();
        return reg.get<TagComponent>(_entity).name;
	}

    bool isActive() const {
        auto& reg = World::Get().Registry();
        return reg.get<ActiveComponent>(_entity).active;
	}
    void SetActive(bool active) {
        auto& reg = World::Get().Registry();
        reg.get<ActiveComponent>(_entity).active = active;
    }
    TransformComponent& GetTransform() {
        auto& reg = World::Get().Registry();
        return reg.get<TransformComponent>(_entity);
	}
    void SetTransform(const TransformComponent& transform) {
        auto& reg = World::Get().Registry();
        reg.get<TransformComponent>(_entity) = transform;
		reg.get<TransformComponent>(_entity).dirty = true;
	}
    void SetPosition(const glm::vec3& position) {
        auto& reg = World::Get().Registry();
        auto& transform = reg.get<TransformComponent>(_entity);
        transform.position = position;
        transform.dirty = true;
	}
    void SetRotation(const glm::quat& rotation) {
        auto& reg = World::Get().Registry();
        auto& transform = reg.get<TransformComponent>(_entity);
        transform.rotation = rotation;
        transform.dirty = true;
    }
    void SetScale(const glm::vec3& scale) {
        auto& reg = World::Get().Registry();
        auto& transform = reg.get<TransformComponent>(_entity);
        transform.scale = scale;
        transform.dirty = true;
	}
    void SetParent(Entity parent) {
        auto& reg = World::Get().Registry();
        auto& transform = reg.get<TransformComponent>(_entity);
        transform.parent = parent;
        transform.dirty = true;
    }
    void SetWorldMatrix(const glm::mat4& worldMatrix) {
        auto& reg = World::Get().Registry();
        auto& transform = reg.get<TransformComponent>(_entity);
        transform.worldMatrix = worldMatrix;
        transform.dirty = true;
	}

	// -- Component management --
	template<typename Component, typename... Args>
    Component& AddComponent(Args&&... args) {
        auto& reg = World::Get().Registry();
        return reg.emplace<Component>(_entity, std::forward<Args>(args)...);
	}

    template<typename Component>
    Component* GetComponent() {
        auto& reg = World::Get().Registry();
        return &reg.try_get<Component>(_entity);
	}

    template<typename Component>
    void RemoveComponent() {
        auto& reg = World::Get().Registry();
        reg.remove<Component>(_entity);
	}

	template<typename Component>
    bool HasComponent() const {
        auto& reg = World::Get().Registry();
        return reg.any_of<Component>(_entity);
	}
private:
	Entity _entity = NullEntity;
};