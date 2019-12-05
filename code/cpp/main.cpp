#ifdef __APPLE__
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "main.h"
#include "input.h"

double getTime() {
  static uint64_t startCount = SDL_GetPerformanceCounter();
  return (SDL_GetPerformanceCounter() - startCount) / (double)SDL_GetPerformanceFrequency();
}

SDL_Renderer *renderer;
int rendererWidth, rendererHeight;

void setWorkingDir() {
  char *path = SDL_GetBasePath();
  string assetsPath = string(path) + "/assets";
#ifdef __APPLE__
  SDL_assert_release(chdir(assetsPath.c_str()) == 0);
#else
  SDL_assert_release(SetCurrentDirectory(assetsPath));
#endif
  SDL_free(path);
}

int main(int argc, char* argv[]) {
  
  #ifdef DEBUG
  printf("Debug build\n");
  printf("Validation enabled\n");
  #else
  printf("Release build\n");
  printf("Validation disabled\n");
  #endif
  
  printf("main()\n");
  fflush(stdout);
  
  int result = SDL_Init(SDL_INIT_EVERYTHING);
  SDL_assert_release(result == 0);
  
  setWorkingDir();

  // create a 4:3 SDL window
  int windowWidth = 600;
  int windowHeight = 600;

  SDL_Window *window = SDL_CreateWindow(
    "Light and Shadow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    windowWidth, windowHeight, SDL_WINDOW_VULKAN);
  SDL_assert_release(window != NULL);
  printf("Created window\n");
  fflush(stdout);
  
  graphics::init(window);
  
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
        case SDL_QUIT: running = false; break;
      }
    }
    
    graphics::updateAndRender(deltaTime);
    // printf("fps: %.1f\n", 1 / deltaTime);
    
    fflush(stdout);
  }
  
  printf("Quitting\n");
  SDL_Quit();
  return 0;
}




