#ifndef KIKI_PBR_GLSL
#define KIKI_PBR_GLSL
const float PI = 3.14159265f;

float masking(vec3 normal, vec3 halfVector, vec3 viewDirection, vec3 lightDirection) {
    float nDotHPos = max(0.f, dot(normal, halfVector));
    float nDotVPos = max(0.f, dot(normal, viewDirection));
    float nDotLPos = max(0.f, dot(normal, lightDirection));

    // clamp so that we aren't dividing by 0
    float vDotH = max(dot(viewDirection, halfVector), 1e-4);

    float a = 2 * ((nDotHPos * nDotVPos) / vDotH);
    float b = 2 * ((nDotHPos * nDotLPos) / vDotH);

    return min(1.f, min(a, b));
}

float normalDistribution(vec3 normal, vec3 halfVector, float roughness) {
    float nDotHPos = max(0.f, dot(normal, halfVector));

    float power = (pow(nDotHPos, 2.f) - 1.f) / (pow(roughness, 2.f) * pow(nDotHPos, 2.f));
    float top = exp(power);

    // clamp so that we aren't dividing by 0
    float bottom = max(PI * pow(roughness, 2.f) * pow(nDotHPos, 4.f), 1e-4);

    return top / bottom;
}

vec3 fresnel(float metalness, vec3 baseColour, vec3 halfVector, vec3 viewDirection) {
    vec3 f0 = ((1 - metalness) * vec3(0.04f)) + (metalness * baseColour);
    return f0 + ((1 - f0) * pow((1 - dot(halfVector, viewDirection)), 5.f));
}

vec3 diffuse(vec3 baseColour, vec3 viewDirection, float metalness, vec3 halfVector) {
    vec3 rhs = (vec3(1.f) - fresnel(metalness, baseColour, halfVector, viewDirection)) * (1.f - metalness);

    return (baseColour / PI) * rhs;
}

vec3 ambient(vec3 sceneAmbient, vec3 baseColour) {
    return sceneAmbient * baseColour;
}

vec3 brdf(vec3 lightDirection, vec3 viewDirection, vec3 normal, vec3 halfVector, float roughness, float metalness, vec3 baseColour) {
    float D = normalDistribution(normal, halfVector, roughness);
    vec3 F = fresnel(metalness, baseColour, halfVector, viewDirection);
    float G = masking(normal, halfVector, viewDirection, lightDirection);

    float nDotVPos = max(0.f, dot(normal, viewDirection));
    float nDotLPos = max(0.f, dot(normal, lightDirection));

    vec3 top = D * F * G;

    // clamp so that we aren't dividing by 0
    float bottom = max(4 * nDotVPos * nDotLPos, 1e-4);

    vec3 frac = top / bottom;

    vec3 lDiffuse = diffuse(baseColour, viewDirection, metalness, halfVector);

    return lDiffuse + frac;
}

#endif