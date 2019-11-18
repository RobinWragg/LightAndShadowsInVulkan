#include "main.h"

namespace graphics {
	const auto requiredSwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
	const auto requiredSwapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const int requiredSwapchainImageCount = 2;
	bool enableVsync = false;
	bool enableDepthTesting = true;

	vector<const char*> requiredValidationLayers = {
#ifdef _DEBUG
		"VK_LAYER_LUNARG_standard_validation"
#endif
	};

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderCompletedSemaphore;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkCommandPool commandPool;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkExtent2D extent;
	int queueFamilyIndex = -1;
	VkDebugUtilsMessengerEXT debugMsgr;
	VkQueue queue = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	vector<VkFramebuffer> framebuffers;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	vector<VkImage> swapchainImages;
	vector<VkImageView> swapchainViews;
	VkDevice device = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkInstance instance = VK_NULL_HANDLE;
	
	vector<uint8_t> loadBinaryFile(const char *filename) {
		ifstream file(filename, ios::ate | ios::binary);

		SDL_assert_release(file.is_open());

		vector<uint8_t> bytes(file.tellg());
		file.seekg(0);
		file.read((char*)bytes.data(), bytes.size());
		file.close();

		return bytes;
	}

	bool deviceHasExtensions(VkPhysicalDevice device, vector<const char*> requiredExtensions) {
		uint32_t availableExtensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions.data());

		for (auto &requiredExt : requiredExtensions) {
			bool found = false;

			for (auto &availableExt : availableExtensions) {
				if (strcmp(requiredExt, availableExt.extensionName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) return false;
		}

		return true;
	}

	bool deviceSupportsAcceptableSwapchain(VkPhysicalDevice device, VkSurfaceKHR surface) {
		
		// Check for required format
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());

		bool foundRequiredFormat = false;

		for (auto &format : formats) {
			if (format.format == requiredSwapchainFormat
				&& format.colorSpace == requiredSwapchainColorSpace) {
				foundRequiredFormat = true;
			}
		}

		return foundRequiredFormat;
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

	VkDeviceQueueCreateInfo buildQueueCreateInfo(VkPhysicalDevice device, VkQueueFlagBits requiredFlags, bool mustSupportSurface) {
		uint32_t familyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
		std::vector<VkQueueFamilyProperties> families(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

		int selectedIndex = -1;

		// Choose the first queue family that has the required support
		for (int index = 0; index < families.size(); index++) {
			if (!requiredFlags || (families[index].queueFlags & requiredFlags)) {
				if (mustSupportSurface) {
					VkBool32 familySupportsSurface;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &familySupportsSurface);

					// Family doesn't support the surface, so skip it
					if (familySupportsSurface == VK_FALSE) continue;
				}

				selectedIndex = index;
				break;
			}
		}

		SDL_assert_release(selectedIndex >= 0);

		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = selectedIndex;
		info.queueCount = 1;

		// I'm allocating without freeing here which is super bad practice,
		// but it's only 4 bytes for the entire life of the program.
		float *priorities = new float[1];
		priorities[0] = 1.0f;
		info.pQueuePriorities = priorities;

		return info;
	}

	void printAvailableInstanceLayers() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		printf("\nAvailable layers:\n");
		for (const auto& layer : availableLayers) printf("\t%s\n", layer.layerName);
	}

	void printAvailableDeviceLayers(VkPhysicalDevice device) {
		uint32_t layerCount;
		vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);
		vector<VkLayerProperties> deviceLayers(layerCount);
		vkEnumerateDeviceLayerProperties(device, &layerCount, deviceLayers.data());

		printf("\nAvailable device layers (deprecated API section):\n");
		for (const auto& layer : deviceLayers) printf("\t%s\n", layer.layerName);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT *data,
		void *pUserData) {

		printf("\n");

		switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: printf("verbose, "); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: printf("info, "); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: printf("WARNING, "); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: printf("ERROR, "); break;
		default: printf("unknown, "); break;
		};

		switch (msgType) {
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: printf("general: "); break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: printf("validation: "); break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: printf("performance: "); break;
		default: printf("unknown: "); break;
		};

		printf("%s (%i objects reported)\n", data->pMessage, data->objectCount);

		switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			SDL_assert_release(false);
			break;
		default: break;
		};

		return VK_FALSE;
	}

	VkPipelineShaderStageCreateInfo buildShaderStage(const char *spirVFilePath, VkShaderStageFlagBits stage) {
		auto spirV = loadBinaryFile(spirVFilePath);

		VkShaderModuleCreateInfo moduleInfo = {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = spirV.size();
		moduleInfo.pCode = (uint32_t*)spirV.data();

		VkShaderModule module = VK_NULL_HANDLE;
		SDL_assert_release(vkCreateShaderModule(device, &moduleInfo, nullptr, &module) == VK_SUCCESS);

		VkPipelineShaderStageCreateInfo stageInfo = {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		stageInfo.stage = stage;

		stageInfo.module = module;
		stageInfo.pName = "main";

		return stageInfo;
	}

	VkAttachmentDescription buildAttachmentDescription(VkFormat format, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout) {
		VkAttachmentDescription attachment = {};

		attachment.format = format;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = storeOp;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = finalLayout;

		return attachment;
	}

	void buildRenderPass() {

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;

		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		vector<VkAttachmentDescription> attachments = {};

		VkAttachmentDescription colorAttachment = buildAttachmentDescription(
			requiredSwapchainFormat, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		attachments.push_back(colorAttachment);

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		if (enableDepthTesting) {
			VkAttachmentDescription depthAttachment = buildAttachmentDescription(
				VK_FORMAT_D32_SFLOAT, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			attachments.push_back(depthAttachment);

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
		}

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		renderPassInfo.attachmentCount = (uint32_t)attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.pDependencies = &dependency;

		SDL_assert_release(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) == VK_SUCCESS);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;
		}

		SDL_assert_release(false);
		return 0;
	}

	void buildVertexBuffers(
		uint32_t particleCount, uint8_t componentCount, float *componentPtrs[],
		vector<VkBuffer> *vertexBuffers, vector<VkDeviceMemory> *vertexBufferMemSlots) {

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(float) * particleCount;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		for (int c = 0; c < componentCount; c++) {
			vertexBuffers->push_back(VkBuffer());
			auto result = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffers->back());
			SDL_assert(result == VK_SUCCESS);

			VkMemoryRequirements memoryReqs;
			vkGetBufferMemoryRequirements(device, vertexBuffers->back(), &memoryReqs);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memoryReqs.size;
			allocInfo.memoryTypeIndex = findMemoryType(
				memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			vertexBufferMemSlots->push_back(VkDeviceMemory());
			result = vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemSlots->back());
			SDL_assert(result == VK_SUCCESS);
			result = vkBindBufferMemory(device, vertexBuffers->back(), vertexBufferMemSlots->back(), 0);
			SDL_assert(result == VK_SUCCESS);

			uint8_t * mappedMemory;
			vkMapMemory(device, vertexBufferMemSlots->back(), 0, bufferInfo.size, 0, (void**)&mappedMemory);
			
			memcpy(mappedMemory, componentPtrs[c], bufferInfo.size);
			vkUnmapMemory(device, vertexBufferMemSlots->back());
		}
		
	}

	void buildPipeline(
		const vector<VkVertexInputBindingDescription> &bindingDescs,
		const vector<VkVertexInputAttributeDescription> &attribDescs) {
		
		vector<VkPipelineShaderStageCreateInfo> shaderStages = {
			buildShaderStage("basic_vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			buildShaderStage("basic_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertexInputInfo.vertexBindingDescriptionCount = (int)bindingDescs.size();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();

		vertexInputInfo.vertexAttributeDescriptionCount = (int)attribDescs.size();
		vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent = extent;

		VkPipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterInfo = {};
		rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterInfo.depthClampEnable = VK_FALSE;
		rasterInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterInfo.lineWidth = 1; // TODO: unnecessary?
		rasterInfo.cullMode = VK_CULL_MODE_NONE;
		rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterInfo.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
		multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingInfo.sampleShadingEnable = VK_FALSE;
		multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		SDL_assert_release(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		pipelineInfo.stageCount = (int)shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportInfo;

		if (enableDepthTesting) {
			VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
			depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilInfo.depthTestEnable = VK_TRUE;
			depthStencilInfo.depthWriteEnable = VK_TRUE;
			depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // Lower depth values mean closer to 'camera'
			depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
			depthStencilInfo.stencilTestEnable = VK_FALSE;
			pipelineInfo.pDepthStencilState = &depthStencilInfo;
		}

		pipelineInfo.pRasterizationState = &rasterInfo;
		pipelineInfo.pMultisampleState = &multisamplingInfo;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		SDL_assert_release(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) == VK_SUCCESS);

		for (auto &stage : shaderStages) vkDestroyShaderModule(device, stage.module, nullptr);
	}

	void buildFramebuffers() {
		framebuffers.resize(swapchainViews.size());

		for (int i = 0; i < swapchainViews.size(); i++) {
			vector<VkImageView> attachments = { swapchainViews[i] };
			if (enableDepthTesting) attachments.push_back(depthImageView);

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = (uint32_t)attachments.size();
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = extent.width;
			framebufferInfo.height = extent.height;
			framebufferInfo.layers = 1;

			SDL_assert_release(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) == VK_SUCCESS);
		}
	}

	void buildSemaphores() {
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS);
		SDL_assert_release(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderCompletedSemaphore) == VK_SUCCESS);
	}

	VkCommandPool buildCommandPool(VkDevice device, int queueFamilyIndex) {
		VkCommandPoolCreateInfo poolInfo = {};

		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = 0;
		poolInfo.pNext = nullptr;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		SDL_assert_release(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) == VK_SUCCESS);
		return commandPool;
	}

	void buildCommandBuffers(
		VkCommandPool commandPool,
		vector<VkBuffer> vertexBuffers,
		uint32_t vertexCount,
		vector<VkCommandBuffer> *commandBuffersOut) {

		commandBuffersOut->resize(framebuffers.size());

		VkCommandBufferAllocateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferInfo.commandPool = commandPool;
		bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // TODO: optimise by reusing commands (secondary buffer level)?
		bufferInfo.commandBufferCount = (int)commandBuffersOut->size();
		auto result = vkAllocateCommandBuffers(device, &bufferInfo, commandBuffersOut->data());
		SDL_assert(result == VK_SUCCESS);

		vector<VkClearValue> clearValues;

		// Color clear value
		clearValues.push_back(VkClearValue());
		clearValues.back().color = { 0, 0, 0, 1 };
		clearValues.back().depthStencil = {};

		if (enableDepthTesting) {
			// Depth/stencil clear value
			clearValues.push_back(VkClearValue());
			clearValues.back().color = {};
			clearValues.back().depthStencil = { 1, 0 };
		}

		for (int i = 0; i < commandBuffersOut->size(); i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // TODO: optimisation possible?
			beginInfo.pInheritanceInfo = nullptr;
			result = vkBeginCommandBuffer((*commandBuffersOut)[i], &beginInfo);
			SDL_assert(result == VK_SUCCESS);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = framebuffers[i];

			renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassInfo.pClearValues = clearValues.data();

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = extent;

			vkCmdBeginRenderPass((*commandBuffersOut)[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline((*commandBuffersOut)[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			vector<VkDeviceSize> offsets = {0,0,0,0};
			vkCmdBindVertexBuffers((*commandBuffersOut)[i], 0, (uint32_t)vertexBuffers.size(), vertexBuffers.data(), offsets.data());

			vkCmdDraw((*commandBuffersOut)[i], vertexCount, 1, 0, 0);

			vkCmdEndRenderPass((*commandBuffersOut)[i]);

			result = vkEndCommandBuffer((*commandBuffersOut)[i]);
			SDL_assert(result == VK_SUCCESS);
		}

		clearValues.resize(0);
	}
	
	VkCommandBuffer buildAndBeginDepthTestingCommandBuffer(VkCommandPool commandPool) {
		SDL_assert_release(commandPool != VK_NULL_HANDLE);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endDepthTestingCommandBuffer(VkCommandBuffer buffer, VkCommandPool commandPool) {
		vkEndCommandBuffer(buffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(device, commandPool, 1, &buffer);
	}

	void setupDepthTesting(VkCommandPool commandPool) {
		SDL_assert_release(commandPool != VK_NULL_HANDLE);

		VkFormat format = VK_FORMAT_D32_SFLOAT;

		// Make depth image
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = extent.width;
		imageInfo.extent.height = extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		SDL_assert_release(vkCreateImage(device, &imageInfo, nullptr, &depthImage) == VK_SUCCESS);

		VkMemoryRequirements memoryReqs;
		vkGetImageMemoryRequirements(device, depthImage, &memoryReqs);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryReqs.size;
		allocInfo.memoryTypeIndex = findMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		SDL_assert_release(vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) == VK_SUCCESS);

		vkBindImageMemory(device, depthImage, depthImageMemory, 0);

		// Make image view
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		SDL_assert_release(vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) == VK_SUCCESS);

		// Initialise image layout
		VkCommandBuffer commandBuffer = buildAndBeginDepthTestingCommandBuffer(commandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = depthImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		endDepthTestingCommandBuffer(commandBuffer, commandPool);
	}

	void init(
		SDL_Window *window,
		const vector<VkVertexInputBindingDescription> &bindingDescs,
		const vector<VkVertexInputAttributeDescription> &attribDescs) {

		printAvailableInstanceLayers();

		vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkPhysicalDeviceFeatures enabledDeviceFeatures = {};

		// Create VK instance
		{
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

			// Specify app info TODO: necessary?
			VkApplicationInfo appInfo = {};
			{
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = "Vulkan Particle System";
				appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.pEngineName = "N/A";
				appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.apiVersion = VK_API_VERSION_1_0;

				createInfo.pApplicationInfo = &appInfo;
			}

			// Request the instance extensions that SDL requires
			vector<const char*> requiredExtensions;
			{
				unsigned int requiredExtensionCount;
				SDL_Vulkan_GetInstanceExtensions(window, &requiredExtensionCount, nullptr);
				requiredExtensions.resize(requiredExtensionCount);
				SDL_Vulkan_GetInstanceExtensions(window, &requiredExtensionCount, requiredExtensions.data());

				if (!requiredValidationLayers.empty()) requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

				createInfo.enabledExtensionCount = (int)requiredExtensions.size();
				createInfo.ppEnabledExtensionNames = requiredExtensions.data();
			}

			// Enable instance validation layers
			createInfo.enabledLayerCount = (int)requiredValidationLayers.size();
			createInfo.ppEnabledLayerNames = requiredValidationLayers.data();

			auto creationResult = vkCreateInstance(&createInfo, nullptr, &instance);
			//SDL_assert_release( == VK_SUCCESS);
			printf("\nCreated Vulkan instance\n");
		}

		// Setup the debug messenger
		if (!requiredValidationLayers.empty()) {
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

			createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
			// createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
			createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			createInfo.pfnUserCallback = debugCallback;

			auto createDebugUtilsMessenger =
				(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			SDL_assert_release(createDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMsgr) == VK_SUCCESS);
		}

		// Create surface
		SDL_assert_release(SDL_Vulkan_CreateSurface(window, instance, &surface));
		printf("\nCreated SDL+Vulkan surface\n");

		// Get physical device (GTX 1060 3GB)
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
			vector<VkPhysicalDevice> candidateDevices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, candidateDevices.data());

			// This picks the first found device that meets my requirements (this resolves to the GTX 1060 3GB on my computer)
			for (auto &candidateDevice : candidateDevices) {
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(candidateDevice, &properties);

				if (VK_VERSION_MAJOR(properties.apiVersion) >= 1
					&& VK_VERSION_MINOR(properties.apiVersion) >= 1
					&& deviceHasExtensions(candidateDevice, requiredDeviceExtensions)
					&& deviceSupportsAcceptableSwapchain(candidateDevice, surface)) {
					physicalDevice = candidateDevice;
					break;
				}
			}

			SDL_assert_release(physicalDevice != VK_NULL_HANDLE);

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			printf("\nChosen device: %s\n", properties.deviceName);
		}

		// Create the logical device with a queue capable of graphics and surface presentation commands
		{
			vector<VkDeviceQueueCreateInfo> queueInfos = {
				buildQueueCreateInfo(physicalDevice, VK_QUEUE_GRAPHICS_BIT, true)
			};
			queueFamilyIndex = queueInfos[0].queueFamilyIndex;

			VkDeviceCreateInfo deviceCreateInfo = {};
			{
				deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

				deviceCreateInfo.pQueueCreateInfos = queueInfos.data();
				deviceCreateInfo.queueCreateInfoCount = (int)queueInfos.size();

				deviceCreateInfo.pEnabledFeatures = &enabledDeviceFeatures;

				// Enable extensions
				deviceCreateInfo.enabledExtensionCount = (int)requiredDeviceExtensions.size();
				deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
			}

			// Enable validation layers for the device, same as the instance (deprecated in Vulkan 1.1, but the API advises we do so) TODO: remove?
			{
				//printAvailableDeviceLayers(physicalDevice);
				//deviceCreateInfo.enabledLayerCount = (int)requiredValidationLayers.size();
				//deviceCreateInfo.ppEnabledLayerNames = requiredValidationLayers.data();
			}

			SDL_assert_release(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) == VK_SUCCESS);
			SDL_assert_release(device != VK_NULL_HANDLE);
			printf("\nCreated logical device\n");

			// Get a handle to the new queue
			int queueIndex = 0; // Only one queue per VkDeviceQueueCreateInfo was created, so this is 0.
			vkGetDeviceQueue(device, queueInfos[0].queueFamilyIndex, queueIndex, &queue);
			SDL_assert_release(queue != VK_NULL_HANDLE);
			printf("\nCreated queue at family index %i\n", queueInfos[0].queueFamilyIndex);
		}
		
		// Create the swapchain
		{
			VkSurfaceCapabilitiesKHR capabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
			extent = capabilities.currentExtent;

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
			vector<VkPresentModeKHR> presentModes(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;
			createInfo.minImageCount = requiredSwapchainImageCount;
			createInfo.imageFormat = requiredSwapchainFormat;
			createInfo.imageColorSpace = requiredSwapchainColorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1; // 1 == not stereoscopic
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Suitable for VkFrameBuffer
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // the graphics and surface queues are the same, so no sharing is necessary.
			createInfo.preTransform = capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Opaque window
			createInfo.presentMode = enableVsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
			createInfo.clipped = VK_FALSE; // Vulkan will always render all the pixels, even if some are osbscured by other windows.
			createInfo.oldSwapchain = VK_NULL_HANDLE; // I will not support swapchain recreation.

			SDL_assert_release(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) == VK_SUCCESS);
			printf("\nCreated swapchain with %i images\n", requiredSwapchainImageCount);
		}

		// Get handles to the swapchain images and create their views
		{
			uint32_t actualImageCount;
			vkGetSwapchainImagesKHR(device, swapchain, &actualImageCount, nullptr);
			SDL_assert_release(actualImageCount >= requiredSwapchainImageCount);
			swapchainImages.resize(actualImageCount);
			swapchainViews.resize(actualImageCount);
			SDL_assert_release(vkGetSwapchainImagesKHR(device, swapchain, &actualImageCount, swapchainImages.data()) == VK_SUCCESS);

			for (int i = 0; i < swapchainImages.size(); i++) {
				VkImageViewCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = swapchainImages[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = requiredSwapchainFormat;
				
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;
				
				SDL_assert_release(vkCreateImageView(device, &createInfo, nullptr, &swapchainViews[i]) == VK_SUCCESS);
			}
		}

		printf("\nInitialised Vulkan\n");

		buildRenderPass();
		buildPipeline(bindingDescs, attribDescs);
		commandPool = buildCommandPool(device, queueFamilyIndex);
		if (enableDepthTesting) setupDepthTesting(commandPool);
		buildFramebuffers();
		buildSemaphores();
	}

	// Ephemeral render buffers
	vector<VkBuffer> vertexBuffers;
	vector<VkDeviceMemory> vertexBufferMemSlots;
	vector<VkCommandBuffer> commandBuffers;

	void freeRenderBuffers() {
		vkFreeCommandBuffers(device, commandPool, (uint32_t)commandBuffers.size(), commandBuffers.data());
		commandBuffers.resize(0);

		for (auto &buffer : vertexBuffers) vkDestroyBuffer(device, buffer, nullptr);
		vertexBuffers.resize(0);

		for (auto &slot : vertexBufferMemSlots) vkFreeMemory(device, slot, nullptr);
		vertexBufferMemSlots.resize(0);
	}

	void render(uint32_t particleCount, uint8_t componentCount, float *componentPtrs[]) {

		if (!commandBuffers.empty()) {
			// The queue may not have finished its commands from the last frame yet,
			// so we wait for everything to be finished before rebuilding the buffers.
			vkQueueWaitIdle(queue);
			freeRenderBuffers();
		}

		buildVertexBuffers(particleCount, componentCount, componentPtrs, &vertexBuffers, &vertexBufferMemSlots);
		buildCommandBuffers(commandPool, vertexBuffers, particleCount, &commandBuffers);

		// Submit commands
		uint32_t swapchainImageIndex = INT32_MAX;

		auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX /* no timeout */, imageAvailableSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
		SDL_assert(result == VK_SUCCESS);
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[swapchainImageIndex];

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.pWaitDstStageMask = &waitStage;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompletedSemaphore;

		result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		SDL_assert(result == VK_SUCCESS);

		// Present
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompletedSemaphore;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &swapchainImageIndex;

		result = vkQueuePresentKHR(queue, &presentInfo);
		SDL_assert(result == VK_SUCCESS);
	}

	void destroy() {
		vkQueueWaitIdle(queue);
		freeRenderBuffers();

		vkDestroyCommandPool(device, commandPool, nullptr);

		if (!requiredValidationLayers.empty()) {
			auto destroyDebugUtilsMessenger =
				(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			destroyDebugUtilsMessenger(instance, debugMsgr, nullptr);
		}

		vkDeviceWaitIdle(device);

		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, renderCompletedSemaphore, nullptr);

		for (auto &buffer : framebuffers) vkDestroyFramebuffer(device, buffer, nullptr);
		
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto view : swapchainViews) vkDestroyImageView(device, view, nullptr);
		swapchainViews.resize(0);

		swapchainImages.resize(0);
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
	}
}



