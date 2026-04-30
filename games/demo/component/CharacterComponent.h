#pragma once
#include <glm/glm.hpp>
enum class CharacterState {
    Idle,
    Walking,
    Running,
    Jumping,
//    Falling,
//    Landing,
};

inline std::string to_string(CharacterState state) {
    switch (state) {
    case CharacterState::Idle:    return "Idle";
    case CharacterState::Walking: return "Walking";
    case CharacterState::Running: return "Running";
    case CharacterState::Jumping: return "Jumping";
    default:                      return "Unknown";
    }
}
enum class Ability {
    Null,
    DoubleJump,
    Dash,
    DoubleSpeed
};
struct CharacterComponent {
    float walkSpeed = 2.0f;
    float runSpeed = 5.0f;
    float jumpForce = 3.0f;
	float rotateSpeed = 10.0f; // character rotation speed (for interpolating facing direction)

    CharacterState state = CharacterState::Idle;
    std::vector< Ability> ablities = { Ability::Null };
    glm::vec3      velocity = { 0, 0, 0 };
	glm::vec3      spawnPosition = { 0, 0, 0 }; // for respawning after falling off the level

	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction
};