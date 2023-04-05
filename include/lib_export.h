#pragma once

#ifdef _WIN32
  #ifdef ChaosRendererEXPORTS
    #define ChaosRendererAPI __declspec(dllexport)
  #else
    #define ChaosRendererAPI __declspec(dllimport)
  #endif
#else
  #define ChaosRendererAPI
#endif

ChaosRendererAPI void render(void* pixels, float t);