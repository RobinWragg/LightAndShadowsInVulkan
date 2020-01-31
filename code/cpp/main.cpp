#ifdef __APPLE__
#include <unistd.h>
#else
#include <direct.h>
#define chdir _chdir
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
  
  scene::init(window);
  
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
    
    scene::updateAndRender(deltaTime);
    // printf("fps: %.1f\n", 1 / deltaTime);
    
    fflush(stdout);
  }
  
  printf("Quitting\n");
  SDL_Quit();
  return 0;
}




