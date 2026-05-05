#pragma once
#include "../GltfLoader/GltfLoaderAssimp.h"

struct SimpleAnimationComponent
{
	MsimpleAnimType type = MsimpleAnimType::NONE;

	glm::vec3 startPosition = glm::vec3(0.0f);
	glm::quat startRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);


	float time = 0.0f;

	float distance = 5.0f;
	float speed = 1.0f;
	float rotationSpeed = 90.0f; // degrees per second

};