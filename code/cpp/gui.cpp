#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"
#include "graphics.h"
#include "settings.h"
#include "shadows.h"

namespace gui {
  SDL_Window *window = nullptr;
  
  using namespace ImGui;
  
  void prepareFont() {
    ImGuiIO &io = GetIO();
    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.0f);
    
    VkCommandBuffer cmdBuffer = gfx::createCommandBuffer();
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
    
    vkEndCommandBuffer(cmdBuffer);
    gfx::submitCommandBuffer(cmdBuffer);

    vkQueueWaitIdle(gfx::queue);
    vkFreeCommandBuffers(gfx::device, gfx::commandPool, 1, &cmdBuffer);
    
    ImGui_ImplVulkan_DestroyFontUploadObjects();
  }
  
  void init(SDL_Window *window_) {
    window = window_;
    
    CreateContext();

    StyleColorsDark();

    ImGui_ImplSDL2_InitForVulkan(window);
    
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = gfx::instance;
    initInfo.PhysicalDevice = gfx::physDevice;
    initInfo.Device = gfx::device;
    initInfo.QueueFamily = gfx::queueFamilyIndex;
    initInfo.Queue = gfx::queue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.MSAASamples = MSAA_SETTING;
    initInfo.DescriptorPool = gfx::descriptorPool;
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = gfx::swapchainSize;
    initInfo.ImageCount = gfx::swapchainSize;
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, gfx::renderPass);
    
    prepareFont();
  }
  
  void processSdlEvent(SDL_Event *event) {
    ImGui_ImplSDL2_ProcessEvent(event);
  }
  
  void render(VkCommandBuffer cmdBuffer) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    NewFrame();
    
    // ShowDemoWindow();
    
    Begin("Lighting Settings");
    
    Checkbox("Animate Lightsource", &settings.animateLightPos);
    
    Text("Subsource Arrangement");
    if (RadioButton("Spiral", settings.subsourceArrangement == settings.SPIRAL)) {
      settings.subsourceArrangement = settings.SPIRAL;
    }
    if (RadioButton("Ring", settings.subsourceArrangement == settings.RING)) {
      settings.subsourceArrangement = settings.RING;
    }
    
    SetNextItemWidth(90);
    InputInt("Subsources", &settings.subsourceCount);
    if (settings.subsourceCount > MAX_LIGHT_SUBSOURCE_COUNT) settings.subsourceCount = MAX_LIGHT_SUBSOURCE_COUNT;
    if (settings.subsourceCount < 1) settings.subsourceCount = 1;
    
    SetNextItemWidth(150);
    SliderFloat("Lightsource Radius", &settings.sourceRadius, 0.01, 1.0, "%.2f");
    
    SetNextItemWidth(90);
    InputInt("Shadow Anti-Alias Kernel Size", &settings.shadowAntiAliasSize, 2);
    
    // Enforce valid values: odd numbers and 0.
    if (settings.shadowAntiAliasSize > MAX_SHADOW_ANTI_ALIAS_SIZE) settings.shadowAntiAliasSize = MAX_SHADOW_ANTI_ALIAS_SIZE;
    if (settings.shadowAntiAliasSize < 0) settings.shadowAntiAliasSize = 0;
    if (settings.shadowAntiAliasSize != 0 && settings.shadowAntiAliasSize % 2 == 0) settings.shadowAntiAliasSize -= 1;
    
    vector<ImVec2> imVecs;
    for (auto &offset : shadows::getViewOffsets()) {
        float radius = settings.sourceRadius;
        imVecs.push_back(ImVec2(offset.x / radius, offset.y / radius));
    }
    
    SetNextItemWidth(90);
    PlotPoints("Subsource Layout Preview", imVecs.data(), (int)imVecs.size(), 0, NULL, FLT_MAX, FLT_MAX, ImVec2(100, 100));
    
    Checkbox("Render textures", &settings.renderTextures);
    Checkbox("Render normalmaps", &settings.renderNormalMaps);
    
    End();
    
    Render();
    ImGui_ImplVulkan_RenderDrawData(GetDrawData(), cmdBuffer);
  }
}





