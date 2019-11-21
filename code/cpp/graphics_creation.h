#pragma once
#include "main.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

extern vector<const char*> layers;

VkInstance createInstance(SDL_Window *window, const vector<const char*> &layers);
void createDebugMessenger(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback);
void destroyDebugMessenger(VkInstance instance);
VkPhysicalDevice createPhysicalDevice(VkInstance instance, SDL_Window *window);
void createLogicalDevice(VkPhysicalDevice physDevice, VkDevice *device, VkQueue *graphicsQueue, VkQueue *surfaceQueue);
