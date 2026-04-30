#pragma once
#include <glm/glm.hpp>
#include "Animation/AnimationComponent.h"

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

    float currentMaxSpeed = walkSpeed;

    float jumpTimer = 0.0f;

    Kiki::CharacterState state = Kiki::CharacterState::Idle;

    //CharacterState state = CharacterState::Idle;
    std::vector< Ability> ablities = { Ability::Null };
    glm::vec3      velocity = { 0, 0, 0 };
	glm::vec3      spawnPosition = { 0, 0, 0 }; // for respawning after falling off the level

	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction
};