#version 450
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fWorldSpace;
layout(location = 3) in vec4 v2fTangent;
layout(location = 4) in vec4 v2fCurrentClip;
layout(location = 5) in vec4 v2fPreviousClip;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;
layout(set = 1, binding = 1) uniform sampler2D uTexRoughnessMetalness;
layout(set = 1, binding = 2) uniform sampler2D uTexNormalMap;

layout(location = 0) out vec4 gTexColour;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec2 gRoughnessMetalness;
layout(location = 3) out vec4 gMappedNormal;
layout(location = 4) out vec2 gVelocity;

layout(push_constant, scalar) uniform PushConstants {
    mat4x3 model;
    mat4x3 previousModel;
    vec4 baseColour;
    vec4 flags;
} object;

vec3 calculateMappedNormal() {
    vec3 normalMapNormal = texture(uTexNormalMap, v2fTexCoord).rgb;
    normalMapNormal = normalize((normalMapNormal * 2.f) - 1.f);

    vec3 N = normalize(v2fNormal);
    vec3 T = normalize(v2fTangent.xyz);
    float sign = v2fTangent.w;

    T = normalize(T - N * dot(T, N));

    // construct bitangent
    vec3 B = normalize(cross(N, T) * sign);

    // build TBN
    mat3 tbn = mat3(T, B, N);

    vec3 mappedNormal = normalize(tbn * normalMapNormal);

    return (mappedNormal * 0.5f) + 0.5f;
}

vec2 calculateVelocity() {
	if (v2fCurrentClip.w <= 0.0 || v2fPreviousClip.w <= 0.0) {
		return vec2(0.0);
	}

	vec2 currentUv = (v2fCurrentClip.xy / v2fCurrentClip.w) * 0.5 + 0.5;
	vec2 previousUv = (v2fPreviousClip.xy / v2fPreviousClip.w) * 0.5 + 0.5;
	return currentUv - previousUv;
}

void main()
{
    float alpha = texture(uTexColor, v2fTexCoord).a;

    if (alpha < 0.5) {
        discard;
    }

    // Beckman roughness = roughness^2
    float roughness = pow(texture(uTexRoughnessMetalness, v2fTexCoord).g, 2.f);
     roughness *= object.flags.z;
    float metalness = texture(uTexRoughnessMetalness, v2fTexCoord).b;
    metalness *= object.flags.w;
    vec3 baseColour = texture(uTexColor, v2fTexCoord).rgb;
        // if not useTexture then use the baseColour
    if (object.flags.y == 0) {
        baseColour = object.baseColour.rgb;
    }
    gTexColour = vec4(baseColour, 1.f);

    gNormal = vec4((normalize(v2fNormal) * 0.5f) + 0.5f, 1.f);
    gRoughnessMetalness = vec2(roughness, metalness);
    gMappedNormal = vec4(calculateMappedNormal(), 1.f);
	gVelocity = calculateVelocity();
}