#pragma once
#include "main.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

extern vector<const char*> layers;

struct GraphicsBaseHandles {
  VkInstance instance                = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;
  VkSurfaceKHR surface               = VK_NULL_HANDLE;
  VkPhysicalDevice physDevice        = VK_NULL_HANDLE;
  VkDevice device                    = VK_NULL_HANDLE;
  VkQueue graphicsQueue              = VK_NULL_HANDLE;
  VkQueue surfaceQueue               = VK_NULL_HANDLE;
};

GraphicsBaseHandles init2(
  SDL_Window *window,
  const vector<const char*> &layers,
  PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackOut);

void destroy(GraphicsBaseHandles handles);

VkSwapchainKHR createSwapchain(const GraphicsBaseHandles &handles);
VkExtent2D getExtent(const GraphicsBaseHandles &handles);





