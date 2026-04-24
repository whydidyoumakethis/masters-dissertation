#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
struct ThirdPersonCameraComponent {
	entt::entity  followTarget = entt::null;  // the entity that the camera will follow (e.g., the player character)

	// camera arm parameters
	float   distance = 5.0f;    // follow distance from the target
    float   minDistance = 1.5f;
    float   maxDistance = 10.0f;
	float   height = 1.8f;    // height offset from the target's position
	float   smoothSpeed = 8.0f;    // smoothing speed for camera movement

	// rotation parameters
	float   yaw = 0.0f; // horizontal angle around the target (in degrees)
	float   pitch = 15.0f;   // default pitch angle (looking slightly down at the target)
    float   minPitch = -20.0f;
    float   maxPitch = 70.0f;
    float   rotateSensitivity = 0.15f;

	// run-time variables
	glm::vec3 currentPos = { 0, 0, 0 };  // current camera position after smoothing
};