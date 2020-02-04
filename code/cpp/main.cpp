#ifdef __APPLE__
#include <unistd.h>
#else
#include <direct.h>
#define chdir _chdir
#endif

#include <string>
#include <fstream>

#include "main.h"
#include "input.h"
#include "imageViewer.h"
#include "graphics.h"
#include "scene.h"

double getTime() {
  static uint64_t startCount = SDL_GetPerformanceCounter();
  return (SDL_GetPerformanceCounter() - startCount) / (double)SDL_GetPerformanceFrequency();
}

SDL_Renderer *renderer;
int rendererWidth, rendererHeight;

void setWorkingDir() {
  char *path = SDL_GetBasePath();
  string assetsPath = string(path) + "/assets";
  auto result = chdir(assetsPath.c_str());
  SDL_assert_release(result == 0);
  SDL_free(path);
}

vector<uint8_t> loadBinaryFile(const char *filename) {
  ifstream file(filename, ios::ate | ios::binary);

  printf("LOADING: %s\n", filename);
  SDL_assert_release(file.is_open());

  vector<uint8_t> bytes(file.tellg());
  file.seekg(0);
  file.read((char*)bytes.data(), bytes.size());
  file.close();

  return bytes;
}

VkSemaphore imageAvailableSemaphore  = VK_NULL_HANDLE;
VkSemaphore renderCompletedSemaphore = VK_NULL_HANDLE;

void createSemaphores() {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(gfx::device, &semaphoreInfo, nullptr, &imageAvailableSemaphore);
  //SDL_assert_release( == VK_SUCCESS);
  SDL_assert_release(vkCreateSemaphore(gfx::device, &semaphoreInfo, nullptr, &renderCompletedSemaphore) == VK_SUCCESS);
}

void beginCommandBuffer(const gfx::SwapchainFrame *frame) {
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = gfx::renderPass;
  renderPassInfo.framebuffer = frame->framebuffer;
  
  vector<VkClearValue> clearValues(2);
  
  clearValues[0].color.float32[0] = 0.5;
  clearValues[0].color.float32[1] = 0.7;
  clearValues[0].color.float32[2] = 1;
  clearValues[0].color.float32[3] = 1;
  
  clearValues[1].depthStencil.depth = 1;
  clearValues[1].depthStencil.stencil = 0;
  
  renderPassInfo.clearValueCount = (int)clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();

  renderPassInfo.renderArea.offset = { 0, 0 };
  renderPassInfo.renderArea.extent = gfx::getSurfaceExtent();
  
  gfx::beginCommandBuffer(frame->cmdBuffer);
  
  vkCmdBeginRenderPass(frame->cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void endCommandBuffer(VkCommandBuffer cmdBuffer) {
  vkCmdEndRenderPass(cmdBuffer);

  auto result = vkEndCommandBuffer(cmdBuffer);
  SDL_assert(result == VK_SUCCESS);
}

void renderNextFrame(float deltaTime) {
  gfx::SwapchainFrame *frame = gfx::getNextFrame(imageAvailableSemaphore);
  
  // Wait for the command buffer to finish executing
  vkWaitForFences(gfx::device, 1, &frame->cmdBufferFence, VK_TRUE, INT64_MAX);
  vkResetFences(gfx::device, 1, &frame->cmdBufferFence);
  
  beginCommandBuffer(frame);
  
  scene::addToCommandBuffer(frame->cmdBuffer, deltaTime);
  imageViewer::render();
  
  endCommandBuffer(frame->cmdBuffer);
  
  // Submit the command buffer
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  gfx::submitCommandBuffer(frame->cmdBuffer, imageAvailableSemaphore, waitStage, renderCompletedSemaphore, frame->cmdBufferFence);
  
  gfx::presentFrame(frame, renderCompletedSemaphore);
}

int main(int argc, char* argv[]) {
  
  #ifdef DEBUG
  printf("Debug build\n");
  printf("Validation enabled\n");
  const char *windowTitle = "Light and Shadow (debug build)";
  #else
  printf("Release build\n");
  printf("Validation disabled\n");
  const char *windowTitle = "Light and Shadow (release build)";
  #endif
  
  printf("main()\n");
  fflush(stdout);
  
  int result = SDL_Init(SDL_INIT_EVERYTHING);
  SDL_assert_release(result == 0);
  
  setWorkingDir();

  // create a window slightly smaller than the display resolution
  int windowWidth;
  int windowHeight;
  {
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    int extraRoom = 200;
    windowWidth = displayMode.w - extraRoom;
    windowHeight = displayMode.h - extraRoom;
  }

  SDL_Window *window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_assert_release(window != NULL);
  printf("Created window\n");
  fflush(stdout);
  
  gfx::createCoreHandles(window);
  createSemaphores();
  
  scene::init();
  
  bool running = true;
  
  printf("Beginning frame loop\n");
  fflush(stdout);
  
  while (running) {
    float deltaTime;
    {
      static double previousTime = 0;
      double timeNow = getTime();
      deltaTime = (float)(timeNow - previousTime);
      previousTime = timeNow;
    }
    
    SDL_Event event;
    input::handleMouseMotion(0, 0);
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_KEYDOWN: {
          if (!event.key.repeat) {
            input::handleKeyPress(event.key.keysym.sym);
          }
        } break;
        case SDL_KEYUP: {
          input::handleKeyRelease(event.key.keysym.sym);
        } break;
        case SDL_MOUSEMOTION: {
          input::handleMouseMotion(event.motion.xrel, event.motion.yrel);
        } break;
        case SDL_MOUSEBUTTONDOWN: {
          input::handleMouseClick(window);
        } break;
        case SDL_QUIT: running = false; break;
      }
    }
    
    renderNextFrame(deltaTime);
    
    fflush(stdout);
  }
  
  printf("Quitting\n");
  SDL_Quit();
  return 0;
}




