#pragma once
#include "main.h"

#include "GraphicsFoundation.h"

extern vector<VkImage> swapchainImages;
extern vector<VkImageView> swapchainViews;

VkSwapchainKHR createSwapchain(const GraphicsFoundation *foundation);

void createSwapchainImagesAndViews(VkSwapchainKHR swapchain, const GraphicsFoundation *foundation);





