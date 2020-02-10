#pragma once

#include "graphics.h"

class ShadowMap {
public:
  VkFormat format;
  uint32_t width, height;
  
  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  
  VkSampler sampler;
  VkDescriptorSetLayout samplerDescriptorSetLayout;
  VkDescriptorSet samplerDescriptorSet;
  
  ShadowMap(uint32_t w, uint32_t h) {
    format = VK_FORMAT_R32G32B32A32_SFLOAT;
    
    width = w;
    height = h;
    
    gfx::createColorImage(width, height, &image, &imageMemory);
    imageView = gfx::createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    
    sampler = gfx::createSampler();
    
    gfx::createDescriptorSet(imageView, sampler, &samplerDescriptorSet, &samplerDescriptorSetLayout);
  }
};





