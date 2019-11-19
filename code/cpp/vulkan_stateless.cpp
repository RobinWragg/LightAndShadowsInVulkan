void printAvailableInstanceLayers() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	printf("\nAvailable layers:\n");
	for (const auto& layer : availableLayers) printf("\t%s\n", layer.layerName);
}

VkApplicationInfo makeApplicationInfo() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Particle System";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "N/A";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	return appInfo;
}

void printQueueFamilies(VkPhysicalDevice device) {
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

	printf("\nDevice has these queue families:\n");
	for (uint32_t i = 0; i < familyCount; i++) {
		printf("\tNumber of queues: %i. Support: ", families[i].queueCount);

		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) printf("graphics ");
		if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) printf("compute ");
		if (families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) printf("transfer ");
		if (families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) printf("sparse_binding ");
		printf("\n");
	}
}

void printAvailableDeviceLayers(VkPhysicalDevice device) {
	uint32_t layerCount;
	vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);
	vector<VkLayerProperties> deviceLayers(layerCount);
	vkEnumerateDeviceLayerProperties(device, &layerCount, deviceLayers.data());

	printf("\nAvailable device layers (deprecated API section):\n");
	for (const auto& layer : deviceLayers) printf("\t%s\n", layer.layerName);
}