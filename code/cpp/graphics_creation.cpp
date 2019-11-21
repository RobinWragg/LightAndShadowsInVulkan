#include "graphics_creation.h"
#include <array>

vector<const char*> layers = {
	// "VK_LAYER_LUNARG_api_dump",
	"VK_LAYER_LUNARG_standard_validation",
	"VK_LAYER_KHRONOS_validation"
};

const auto requiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
const auto requiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkSurfaceKHR surface = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;

enum class QueueType {
  GRAPHICS,
  SURFACE_SUPPORT
};

void printAvailableInstanceLayers() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	printf("\nAvailable instance layers:\n");
	for (const auto& layer : availableLayers) printf("\t%s\n", layer.layerName);
}

VkInstance createInstance(SDL_Window *window, const vector<const char*> &layers) {
	if (!layers.empty()) printAvailableInstanceLayers();
	
	// Get required extensions from SDL
	unsigned int extensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
	vector<const char*> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
	
	// Add debug utils if layers are being used
	if (!layers.empty()) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledExtensionCount = (int)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	// Enable the layers
	createInfo.enabledLayerCount = (int)layers.size();
	createInfo.ppEnabledLayerNames = layers.data();
	
	VkInstance instance;
	auto creationResult = vkCreateInstance(&createInfo, nullptr, &instance);
	SDL_assert_release(creationResult == VK_SUCCESS);
	
	return instance;
}

void createDebugMessenger(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback) {
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	// createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
	createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = callback;

	auto createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	
	auto result = createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMsgr);
	SDL_assert_release(result == VK_SUCCESS);
}

void destroyDebugMessenger(VkInstance instance) {
	auto destroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	destroyDebugUtilsMessenger(instance, debugMsgr, nullptr);
}

VkPhysicalDevice createPhysicalDevice(VkInstance instance, SDL_Window *window) {
	SDL_assert_release(surface == VK_NULL_HANDLE);
	SDL_assert_release(SDL_Vulkan_CreateSurface(window, instance, &surface) == SDL_TRUE);
	
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	vector<VkPhysicalDevice> availableDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());
	
	vector<VkPhysicalDevice> suitableDevices;
	
	for (auto &availableDevice : availableDevices) {
		
		// Require Vulkan 1.x
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(availableDevice, &properties);
			if (VK_VERSION_MAJOR(properties.apiVersion) != 1) continue;
		}
		
		// Check for required extensions
		{
			uint32_t count;
			vkEnumerateDeviceExtensionProperties(availableDevice, nullptr, &count, nullptr);
			vector<VkExtensionProperties> availableExtensions(count);
			vkEnumerateDeviceExtensionProperties(availableDevice, nullptr, &count, availableExtensions.data());
			
			bool deviceHasRequiredExtensions = true;
			
			for (auto &requiredExt : deviceExtensions) {
				bool foundExtension = false;
				
				for (auto &availableExt : availableExtensions) {
					if (strcmp(requiredExt, availableExt.extensionName) == 0) {
						foundExtension = true;
						break;
					}
				}
				
				if (!foundExtension) {
					deviceHasRequiredExtensions = false;
					break;
				}
			}
			
			if (!deviceHasRequiredExtensions) continue;
		}
		
		// Check for required format and colour space
		{
			uint32_t count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &count, nullptr);
			vector<VkSurfaceFormatKHR> formats(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(availableDevice, surface, &count, formats.data());

			bool found = false;
			for (auto &format : formats) {
				if (format.format == requiredFormat
					&& format.colorSpace == requiredColorSpace) {
					found = true;
				}
			}
			
			if (!found) continue;
		}

		suitableDevices.push_back(availableDevice);
	}
	
	
	SDL_assert_release(suitableDevices.size() == 1);
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(suitableDevices[0], &properties);
	printf("\nChosen device: %s\n", properties.deviceName);
	return suitableDevices[0];
}

VkDeviceQueueCreateInfo createQueueInfo(QueueType queueType, VkPhysicalDevice physDevice) {
	SDL_assert_release(surface != VK_NULL_HANDLE);
	
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &familyCount, families.data());

	int selectedIndex = -1;

	// Choose the first queue family of the required type
	for (int familyIndex = 0; familyIndex < families.size(); familyIndex++) {
		switch (queueType) {
			case QueueType::GRAPHICS:
				if (families[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					selectedIndex = familyIndex;
					break;
				}
			break;
			case QueueType::SURFACE_SUPPORT:
				VkBool32 hasSurfaceSupport;
				vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, familyIndex, surface, &hasSurfaceSupport);
				
				if (hasSurfaceSupport == VK_TRUE) {
					selectedIndex = familyIndex;
					break;
				}
			break;
			default: SDL_assert_release(false); break;
		}
	}

	SDL_assert_release(selectedIndex >= 0);

	VkDeviceQueueCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	info.queueFamilyIndex = selectedIndex;
	info.queueCount = 1;
	
	static float defaultPriority = 1.0f;
	info.pQueuePriorities = &defaultPriority;
	
	return info;
}

void createLogicalDevice(VkPhysicalDevice physDevice, VkDevice *device, VkQueue *graphicsQueue, VkQueue *surfaceQueue) {
	
	auto graphicsQueueInfo = createQueueInfo(QueueType::GRAPHICS, physDevice);
	auto surfaceQueueInfo = createQueueInfo(QueueType::SURFACE_SUPPORT, physDevice);
	vector<VkDeviceQueueCreateInfo> queueInfos = {graphicsQueueInfo};
	
	if (surfaceQueueInfo.queueFamilyIndex != queueInfos[0].queueFamilyIndex) {
		queueInfos.push_back(surfaceQueueInfo);
	}
	
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	
	deviceCreateInfo.queueCreateInfoCount = (int)queueInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueInfos.data();
	
	VkPhysicalDeviceFeatures enabledDeviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &enabledDeviceFeatures;
	
	// Enable extensions
	deviceCreateInfo.enabledExtensionCount = (int)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	
	// Enable validation layers for the device, same as the instance
	deviceCreateInfo.enabledLayerCount = (int)layers.size();
	deviceCreateInfo.ppEnabledLayerNames = layers.data();
	
	auto result = vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, device);
	SDL_assert_release(result == VK_SUCCESS);
	
	// Get handles to the new queue(s)
	int queueIndex = 0; // Only one queue per VkDeviceQueueCreateInfo was created, so this is 0.
	
	vkGetDeviceQueue(*device, graphicsQueueInfo.queueFamilyIndex, queueIndex, graphicsQueue);
	SDL_assert_release(*graphicsQueue != VK_NULL_HANDLE);
	
	vkGetDeviceQueue(*device, surfaceQueueInfo.queueFamilyIndex, queueIndex, surfaceQueue);
	SDL_assert_release(*surfaceQueue != VK_NULL_HANDLE);
}







