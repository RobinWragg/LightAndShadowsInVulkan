#version 450

layout(location = 0) in vec3 vertPosInWorld;
layout(location = 1) in vec3 interpVertNormalInWorld;
layout(location = 2) in vec3 lightPosInWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform LightViewMatrix {
  mat4 value;
} lightViewMatrix;

layout(set = 1, binding = 0) uniform LightProjMatrix {
  mat4 value;
} lightProjMatrix;

layout(set = 4, binding = 0) uniform sampler2D shadowMap;

// Returns the degree to which a world position is shadowed.
// 0 for no shadow, 1 for completely shadowed.
float getShadowFactor(vec3 posInWorld) {
  vec3 posInLightView = (lightViewMatrix.value * vec4(posInWorld, 1)).xyz;
  
  float texelSize = 1.0 / textureSize(shadowMap, 0).x;
  
  const vec4 posInLightProj = lightProjMatrix.value * vec4(posInLightView, 1);
  
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
      
      // Only the R value is used because the other components are used for debugging.
      const float lightTravelDistance = texture(shadowMap, texCoordWithOffset).r;
      
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






