
const auto requiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
const auto requiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
const char *deviceExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

VkDebugUtilsMessengerEXT debugMsgr = VK_NULL_HANDLE;

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

VkPhysicalDevice createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
	
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
		
		// Check for the swapchain extension
		{
			uint32_t count;
			vkEnumerateDeviceExtensionProperties(availableDevice, nullptr, &count, nullptr);
			vector<VkExtensionProperties> availableExtensions(count);
			vkEnumerateDeviceExtensionProperties(availableDevice, nullptr, &count, availableExtensions.data());
			
			bool found = false;
			for (auto &availableExt : availableExtensions) {
				if (strcmp(availableExt.extensionName, deviceExtension) == 0) {
					found = true;
					break;
				}
			}
			
			if (!found) continue;
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







