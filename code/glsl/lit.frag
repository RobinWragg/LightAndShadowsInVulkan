#version 450

layout(location = 0) in vec3 vertPosInWorld;
layout(location = 1) in vec3 interpVertNormalInWorld;
layout(location = 2) in vec3 lightPosInWorld;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform LightMatrices {
  mat4 view;
  mat4 proj;
} lightMatrices;

layout(set = 3, binding = 0) uniform LightViewOffsets {
  vec2 values[6];
} lightViewOffsets;

layout(push_constant) uniform Config {
  int shadowMapCount;
} config;

layout(set = 4, binding = 0) uniform sampler2D shadowMap0;
layout(set = 5, binding = 0) uniform sampler2D shadowMap1;
layout(set = 6, binding = 0) uniform sampler2D shadowMap2;
layout(set = 7, binding = 0) uniform sampler2D shadowMap3;
layout(set = 8, binding = 0) uniform sampler2D shadowMap4;
layout(set = 9, binding = 0) uniform sampler2D shadowMap5;

float getShadowMapTexel(int index, vec2 texCoord) {
  switch (index) {
    case 0: return texture(shadowMap0, texCoord).r;
    case 1: return texture(shadowMap1, texCoord).r;
    case 2: return texture(shadowMap2, texCoord).r;
    case 3: return texture(shadowMap3, texCoord).r;
    case 4: return texture(shadowMap4, texCoord).r;
    case 5: return texture(shadowMap5, texCoord).r;
  };
  return 0;
}

// Returns the degree to which a world position is shadowed.
// 0 for no shadow, 1 for completely shadowed.
float getShadowFactorFromMap(vec3 posInWorld, int shadowMapIndex) {
  vec3 posInLightView = (lightMatrices.view * vec4(posInWorld, 1)).xyz;
  posInLightView.xy += lightViewOffsets.values[shadowMapIndex];
  
  // All shadowmaps have the same dimensions, so we just get the size of shadowmap 0.
  float texelSize = 1.0 / textureSize(shadowMap0, 0).r;
  
  const vec4 posInLightProj = lightMatrices.proj * vec4(posInLightView, 1);
  
  // This is the perspective division that transforms projection space into normalised device space.
  const vec3 normalisedDevicePos = posInLightProj.xyz / posInLightProj.w;
  
  // Change the bounds from [-1,1] to [0,1].
  vec2 centreTexCoord = normalisedDevicePos.xy * 0.5 + 0.5;
  
  const float posToLightPosDistance = length(posInLightView);
  
  // This is necessary due to floating point inaccuracy. This equates to a tenth of a millimeter in world/lightView space, so it's not noticeable.
  const float epsilon = 0.0001;
  
  int totalSampleCount = 0;
  int shadowSampleCount = 0;
  
  // Use a 5x5 sample kernel.
  for (int x = -0; x <= 0; x++) {
    for (int y = -0; y <= 0; y++) {
      totalSampleCount++;
      
      const vec2 texCoordWithOffset = centreTexCoord + vec2(x, y) * texelSize;
      
      const float lightTravelDistance = getShadowMapTexel(shadowMapIndex, texCoordWithOffset);
      
      if (posToLightPosDistance - lightTravelDistance > epsilon) {
        shadowSampleCount++;
      }
    }
  }
  
  return float(shadowSampleCount) / totalSampleCount;
}

float getTotalShadowFactor(vec3 posInWorld) {
  float totalFactor = 0;
  
  for (int i = 0; i < config.shadowMapCount; i++) {
    totalFactor += getShadowFactorFromMap(posInWorld, i);
  }
  
  return totalFactor / config.shadowMapCount;
}

void main() {
  // Interpolation can cause normals to be non-unit length, so we re-normalise them here
  vec3 vertNormalInWorld = normalize(interpVertNormalInWorld);
  
  const vec3 vertToLightVector = lightPosInWorld - vertPosInWorld;
  
  float lightNormalDot = dot(vertNormalInWorld, normalize(vertToLightVector));
  
  vec3 preShadowColor = vec3(1, 1, 1) * lightNormalDot;
  outColor = vec4(preShadowColor, 1);
  
  if (lightNormalDot > 0) {
    // Attenuate the fragment color if it is in shadow
    float maxLightAttenuation = 0.7;
    float lightAttenuation = getTotalShadowFactor(vertPosInWorld) * maxLightAttenuation;
    outColor.rgb *= 1 - lightAttenuation;
  }
}






