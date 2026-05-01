#pragma once
#include <glm/glm.hpp>
#include "Animation/AnimationComponent.h"

enum class Ability : uint32_t {
    None = 0,
    DoubleJump = 1 << 0,  // 0b0001
    Dash = 1 << 1,  // 0b0010
    SpeedBoost = 1 << 2,  // 0b0100
    //WallJump = 1 << 3,  // 0b1000
    //Glide = 1 << 4,  // 0b10000
};
inline Ability operator|(Ability a, Ability b) {
    return static_cast<Ability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline Ability operator&(Ability a, Ability b) {
    return static_cast<Ability>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline Ability operator~(Ability a) {
    return static_cast<Ability>(~static_cast<uint32_t>(a));
}
inline Ability& operator|=(Ability& a, Ability b) {
    return a = a | b;
}
inline Ability& operator&=(Ability& a, Ability b) {
    return a = a & b;
}
struct CharacterComponent {
    float walkSpeed = 2.0f;
    float runSpeed = 5.0f;
    float jumpForce = 3.0f;
	float rotateSpeed = 10.0f; // character rotation speed (for interpolating facing direction)

    float currentMaxSpeed = walkSpeed;

    float jumpTimer = 0.0f;

    Kiki::CharacterState state = Kiki::CharacterState::Idle;

    //CharacterState state = CharacterState::Idle;
    Ability abilities = Ability::None;
    glm::vec3      velocity = { 0, 0, 0 };
	glm::vec3      spawnPosition = { 0, 0, 0 }; // for respawning after falling off the level

	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction

    bool hasAbility(Ability ability) const {
        return (abilities & ability) == ability;
    }

    void grantAbility(Ability ability) {
        abilities |= ability;
    }

    void revokeAbility(Ability ability) {
        abilities &= ~ability;
    }
};