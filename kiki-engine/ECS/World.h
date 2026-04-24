#pragma once
#include <entt/entt.hpp>
#include <unordered_map>
#include <string>
using Entity = entt::entity;
constexpr Entity NullEntity = entt::null;
class World {
public:
	static World& Get() {
		static World instance;
		return instance;
	}
	entt::registry& Registry() {
		return _registry;
	}
	Entity CreateEntity( ) {
		Entity e = _registry.create();
		return e;
	}
	void DestroyEntity(Entity e) {
		_pendingDestroy.push_back(e);
	}
	void FlushDestroy() {
		for (Entity e : _pendingDestroy)
			_registry.destroy(e);
		_pendingDestroy.clear();
	}
	template<typename... Components>
	auto Query() {
		return _registry.view<Components...>();
	}
	template<typename... Components>
	auto GetComponent(Entity e) {
		return _registry.try_get<Components...>(e);
	}
private:
	World() = default;
	entt::registry _registry;
	std::vector<Entity>      _pendingDestroy;
};