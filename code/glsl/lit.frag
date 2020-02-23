#version 450

bool PERCENTAGE_CLOSER_FILTERING = true;

layout(location = 0) in vec3 vertPosInWorld;
layout(location = 1) in vec3 vertNormalInWorld;
layout(location = 2) in vec3 lightPosInWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform LightViewMatrix {
  mat4 value;
} lightViewMatrix;

layout(set = 1, binding = 0) uniform LightProjMatrix {
  mat4 value;
} lightProjMatrix;

layout(set = 4, binding = 0) uniform sampler2D shadowMap;

float getLightTravelDistance(vec4 posInLightProj, vec2 normalisedDeviceOffset) {
  
  // This is the perspective division that transforms projection space into normalised device space.
  vec3 normalisedDevicePos = posInLightProj.xyz / posInLightProj.w;
  
  // Change the bounds from [-1,1] to [0,1].
  vec2 texCoord = (normalisedDevicePos.xy + normalisedDeviceOffset) * 0.5 + 0.5;
  
  // Only the R value is used because the other components are used for debugging.
  return texture(shadowMap, texCoord).r;
}

bool posIsInShadow(vec3 posInLightView, vec2 normalisedDeviceOffset) {
  vec4 posInLightProj = lightProjMatrix.value * vec4(posInLightView, 1);
  
  float lightTravelDistance = getLightTravelDistance(posInLightProj, normalisedDeviceOffset);
  
  float posToLightPosDistance = length(posInLightView);
  float epsilon = 0.0001; // This is necessary due to floating point inaccuracy
  return posToLightPosDistance - lightTravelDistance > epsilon;
}

// Returns the degree to which a world position is shadowed.
// 0 for no shadow, 1 for completely shadowed.
float getShadowFactor(vec3 posInWorld) {
  vec3 posInLightView = (lightViewMatrix.value * vec4(posInWorld, 1)).xyz;
  
  if (PERCENTAGE_CLOSER_FILTERING) {
    float texelSize = 1.0 / textureSize(shadowMap, 0).x;
    
    int totalSampleCount = 0;
    int shadowSampleCount = 0;
    
    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        totalSampleCount++;
        
        vec2 offset = vec2(x, y) * texelSize;
        if (posIsInShadow(posInLightView, offset)) {
          shadowSampleCount++;
        }
      }
    }
    
    return float(shadowSampleCount) / totalSampleCount;
  } else {
    return posIsInShadow(posInLightView, vec2(0, 0)) ? 1 : 0;
  }
}

void main() {
  
  const vec3 vertToLightVector = lightPosInWorld - vertPosInWorld;
  
  vec3 preShadowColor = vec3(1, 1, 1) * dot(vertNormalInWorld, normalize(vertToLightVector));
  outColor = vec4(preShadowColor, 1);
  
  // Attenuate the fragment color if it is in shadow
  float maxLightAttenuation = 0.7;
  float lightAttenuation = getShadowFactor(vertPosInWorld) * maxLightAttenuation;
  outColor.xyz *= 1 - lightAttenuation;
}






