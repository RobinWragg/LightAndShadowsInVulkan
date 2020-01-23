#include "graphics.h"

namespace gfx {
  void setMemory(VkDeviceMemory memory, uint64_t dataSize, const void *data) {  
    uint8_t *mappedMemory;
    auto result = vkMapMemory(device, memory, 0, dataSize, 0, (void**)&mappedMemory);
    SDL_assert(result == VK_SUCCESS);
        
    memcpy(mappedMemory, data, dataSize);
    vkUnmapMemory(device, memory);
  }
}








