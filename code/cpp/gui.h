#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

namespace gui {
  void init(SDL_Window *window);
  void processSdlEvent(SDL_Event *event);
  void render(VkCommandBuffer cmdBuffer);
}