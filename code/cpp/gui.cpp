#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"
#include "graphics.h"
#include "settings.h"

namespace gui {
  SDL_Window *window = nullptr;
  
  using namespace ImGui;
  
  void prepareFont() {
    ImGuiIO &io = GetIO();
    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 20.0f);
    
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
    
    static bool checkboxState = false;
    Checkbox("My checkbox", &checkboxState);
    
    static int radioValue = 0;
    RadioButton("2048", &radioValue, 2048);
    RadioButton("4096", &radioValue, 4096);
    
    Text("Subsource Arrangement");
    if (RadioButton("Spiral", settings.subsourceArrangement == settings.SPIRAL)) {
      settings.subsourceArrangement = settings.SPIRAL;
    }
    if (RadioButton("Ring", settings.subsourceArrangement == settings.RING)) {
      settings.subsourceArrangement = settings.RING;
    }
    
    SetNextItemWidth(90);
    InputInt("Subsources", &settings.subsourceCount);
    if (settings.subsourceCount > MAX_SUBSOURCE_COUNT) settings.subsourceCount = MAX_SUBSOURCE_COUNT;
    if (settings.subsourceCount < 1) settings.subsourceCount = 1;
    
    SetNextItemWidth(150);
    SliderFloat("Lightsource Radius", &settings.sourceRadius, 0.01, 0.5, "%.2f");
    
    End();
    
    Render();
    ImGui_ImplVulkan_RenderDrawData(GetDrawData(), cmdBuffer);
  }
}




