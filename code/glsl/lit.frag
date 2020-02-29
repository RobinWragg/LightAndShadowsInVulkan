#version 450

layout(location = 0) in vec3 vertPosInWorld;
layout(location = 1) in vec3 interpVertNormalInWorld;
layout(location = 2) in vec3 lightPosInWorld;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform LightMatrices {
  mat4 view;
  mat4 proj;
} lightMatrices;

layout(set = 3, binding = 0) uniform sampler2D shadowMap0;
layout(set = 4, binding = 0) uniform sampler2D shadowMap1;
layout(set = 5, binding = 0) uniform sampler2D shadowMap2;
layout(set = 6, binding = 0) uniform sampler2D shadowMap3;
layout(set = 7, binding = 0) uniform sampler2D shadowMap4;
layout(set = 8, binding = 0) uniform sampler2D shadowMap5;

// Returns the degree to which a world position is shadowed.
// 0 for no shadow, 1 for completely shadowed.
float getShadowFactor(vec3 posInWorld) {
  vec3 viewOffsetTODO = vec3(0, 0, 0);
  vec3 posInLightView = (lightMatrices.view * vec4(posInWorld, 1)).xyz + viewOffsetTODO;
  
  float texelSize = 1.0 / textureSize(shadowMap0, 0).x;
  
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
  for (int x = -2; x <= 2; x++) {
    for (int y = -2; y <= 2; y++) {
      totalSampleCount++;
      
      const vec2 texCoordWithOffset = centreTexCoord + vec2(x, y) * texelSize;
      
      const float lightTravelDistance = texture(shadowMap0, texCoordWithOffset).r;
      
      if (posToLightPosDistance - lightTravelDistance > epsilon) {
        shadowSampleCount++;
      }
    }
  }
  
  return float(shadowSampleCount) / totalSampleCount;
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
    float lightAttenuation = getShadowFactor(vertPosInWorld) * maxLightAttenuation;
    outColor.rgb *= 1 - lightAttenuation;
  }
}






